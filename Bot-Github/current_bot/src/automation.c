#include "automation.h"
#include "protocol.h"
#include "tech_research.h"
#include "items.h"
#include "log.h"
#include "activity_log.h"
#include "gathering.h"
#include "bot_settings.h"
#include <time.h>
#include <stdlib.h>
#include <string.h>

/* Real per-step research costs decoded from GameAssets/TechLv.txt.  The wire
   does not carry costs, and a research step that costs more food/gold than the
   account holds can never land — sending it just produces the recurring
   3203 result=5 reject.  Advisory only: a lookup miss (=unknown tech/level)
   returns NULL and the send is allowed as before. */
#include "game_table_costs.h"
/* Exact per-step building costs decoded from GameAssets/buildUP.txt.  The wire
   does not carry costs; a build step that costs more of any resource than the
   account holds can never land (2003 -> 2013).  Advisory only: a lookup miss
   (=unknown build/level) returns NULL and the send is allowed as before. */
#include "game_build_cost.h"
/* Item catalog decoded from GameAssets/Item.txt.  Amount = amount_scalar *
   amount_mult (VALIDATED 7/7 against the bot's own hardcoded resource-pack
   amounts, e.g. FOOD_150K=1014 -> 150*1000=150000).  Used to source bag
   top-up pack restore values from GameAssets instead of the hardcoded list. */
#include "game_item.h"
/* Soldier catalog decoded from GameAssets/Soldier.txt.  ONLY the tier grouping
   (u16@26, tiers 1..7, ids grouped 4-per-tier) is structurally validated; the
   food/load/unk fields carry an explicit UNVALIDATED warning and are NOT used
   to drive any send.  Here it backs a data-driven tier clamp (advisory only,
   like the cost gates) and an observable load-confirmation log line. */
#include "game_soldier.h"

static uint32_t now32(void) { return (uint32_t)time(NULL); }

/* Level of the player's castle (BUILD_CASTLE tile).  In Lords Mobile a building
   cannot exceed the castle's level, so this is the cap for every upgrade.  0 if
   the castle tile is not present (then the cap gate is skipped, server decides). */
static uint8_t castle_level(Connection *c)
{
    for (uint16_t i = 0; i < c->building_count; ++i)
        if (c->building[i].build_id == BUILD_CASTLE)
            return c->building[i].level;
    return 0;
}

/* Bag-resource top-up helpers (defined later in this file; forward-declared so
   AutoBuildingTick / AutoResearchTick can gate on the floor). */
static uint32_t ResourceBalance(Connection *c, ResourceType rt);
static uint16_t PickResourceItem(Connection *c, ResourceType rt, uint32_t need);
static bool TopUpResourceFromBag(Connection *c, ResourceType rt, uint32_t floor,
                                 uint32_t *last_use, uint32_t now);
static bool EnsureBuildResearchFloor(Connection *c);
/* True when the bag still holds a pack of a resource that is below rss_floor
   for the build/research gate (wood or rock).  Used to decide whether a defer
   is just a refill in flight (retry soon) — vs. the bag is truly empty, in
   which case the tile should be skipped so the bot moves on. */
static bool BagCanStillBridge(Connection *c);

/* TroopType enum label (0-based, mirrors RequestTroopTraining). */
static const char *troop_name(uint8_t kind)
{
    static const char *const names[4] = { "Infantry", "Ranged", "Cavalry", "Siege" };
    return (kind < 4) ? names[kind] : "Troop";
}

static bool queue_busy(uint64_t finish, uint64_t server_time)
{
    return finish != 0 && finish > server_time;
}

static void AutoBuildingTick(Connection *c)
{
    /* Use settings from bot_settings */
    if (!g_BotSettings.build.autoBuild || c->building_count == 0) return;
    uint32_t now = now32();
    if (now - c->automation.building_last_action < 3) return;

    if (c->automation.building_queue_active) {
        if (c->automation.building_finish_time &&
            !queue_busy(c->automation.building_finish_time, c->server_time)) {
            RequestBuildCompleteFree(c,
                                     (uint8_t)c->automation.building_position_id);
            ActivityLogPush(c, ACT_BUILD_SPEED, "Building Speed Up, New Time: Finished");
            c->automation.building_last_action = now;
        }
        return;
    }

    int selected = -1;
    for (int i = 0; i < c->building_count; ++i) {
        BuildingInfo *b = &c->building[i];
        if (!IsBuilding(b->build_id)) continue;
        if (b->level >= c->automation.building_max_level) continue;
        /* A tile the server rejected (RESP_BUILDINGERROR 2013) is skipped
           until building_skip_until so we do not hammer it every 3s. */
        if (now < c->automation.building_skip_until &&
            b->pos_x == c->automation.building_skip_x &&
            b->pos_y == c->automation.building_skip_y) continue;
        if (selected < 0 ||
            (c->automation.building_lowest_level_first && b->level < c->building[selected].level)) {
            selected = i;
        }
    }
    if (selected < 0) return;

    BuildingInfo *b = &c->building[selected];

    /* GameAssets cost + castle-cap gates (mirror the validated research gates).
       The wire carries no costs, so a step the account cannot pay for, or a
       building that has hit the castle cap, would only be sent to be rejected
       (2003 -> 2013 body=03) every 3s.  A lookup miss (=unknown build/level)
       forwards to the server as before, so nothing valid is ever blocked. */
    uint8_t castle = castle_level(c);
    /* Look up the exact next-step cost UNCONDITIONALLY.  We do NOT gate the
       lookup on c->resource_loaded: that flag dip is exactly what allowed one
       blind 2003 through this gate before (a momentarily-unloaded snapshot
       turned gb NULL and bypassed BOTH checks, so the server rejected Barracks
       L24->25 we had already deferred ~130 times).  With resources unloaded,
       c->resources is all-zero, so the cost check below reads a shortfall and
       waits a cycle — safer than sending blind.  An unknown build (NULL) still
       forwards, so nothing valid is ever blocked. */
    const GameBuildCost *gb = game_build_cost(b->build_id, (uint8_t)(b->level + 1));
    /* Castle cap (behavior rule): a building's target level cannot exceed the
       castle's own level.  This is the confirmed 2013 body=03 blocker on Barracks
       L24->25 (costs ~6.3M, affordable — the reject is the cap, not money).  The
       castle itself upgrades freely. */
    if (gb != NULL && b->build_id != BUILD_CASTLE && castle != 0 &&
        (uint8_t)(b->level + 1) > castle) {
        LOGI("[AUTO][BUILD] skip %s tile=(%u,%u) level=%u->%u: castle is Lv%u (cap) — deferring\n",
             GetBuildingName(b->build_id), (unsigned)b->pos_x, (unsigned)b->pos_y,
             b->level, (unsigned)(b->level + 1), (unsigned)castle);
        c->automation.building_skip_x = b->pos_x;
        c->automation.building_skip_y = b->pos_y;
        c->automation.building_skip_until = now + 120;
        c->automation.building_last_action = now;
        return;
    }
    /* Exact-cost gate: skip a step the account demonstrably cannot pay now for
       ANY of the five resources.  A bag top-up is a separate async request, so
       it does not count as paid here; only send when the stock truly covers it.
       The optional bag top-up below still runs first so an affordable-but-short
       step gets funded before we re-examine it next cycle. */
    if (gb != NULL &&
        (c->resources.food < gb->food || c->resources.rock < gb->rock ||
         c->resources.wood < gb->wood || c->resources.ore < gb->ore ||
         c->resources.gold < gb->gold)) {
        LOGI("[AUTO][BUILD] skip %s tile=(%u,%u) level=%u->%u: needs food=%u rock=%u wood=%u ore=%u gold=%u, have food=%u rock=%u wood=%u ore=%u gold=%u (unpayable) — deferring\n",
             GetBuildingName(b->build_id), (unsigned)b->pos_x, (unsigned)b->pos_y,
             b->level, (unsigned)(b->level + 1),
             (unsigned)gb->food, (unsigned)gb->rock, (unsigned)gb->wood,
             (unsigned)gb->ore, (unsigned)gb->gold,
             (unsigned)c->resources.food, (unsigned)c->resources.rock,
             (unsigned)c->resources.wood, (unsigned)c->resources.ore,
             (unsigned)c->resources.gold);
        c->automation.building_skip_x = b->pos_x;
        c->automation.building_skip_y = b->pos_y;
        c->automation.building_skip_until = now + 60;
        c->automation.building_last_action = now;
        return;
    }

    /* Best-effort bag top-up: after the exact-cost gate above has cleared (so the
       account is genuinely able to pay), if the stock is still below the guessed
       rss_floor, spend a matching pack to raise it.  Opportunistic — never a gate.
       Send 2003 and let the server accept or reject; the 2013 handler skips a
       truly-bad tile for 120s and moves on, so we cannot loop. */
    if (EnsureBuildResearchFloor(c)) {
        LOGI("[AUTO][BUILD] (bag top-up ok) %s tile=(%u,%u) level=%u -> %u\n",
             GetBuildingName(b->build_id), b->pos_x, b->pos_y,
             b->level, (unsigned)(b->level + 1));
    }
    LOGI("[AUTO][BUILD] upgrading %s tile=(%u,%u) level=%u -> %u\n",
         GetBuildingName(b->build_id), b->pos_x, b->pos_y,
         b->level, (unsigned)(b->level + 1));
    SendStartBuilding(c, b->pos_x, b->pos_y, b->build_id, 2); /* kUpgrade */
    {
        char ad[120];
        snprintf(ad, sizeof ad, "Build/Upgrade %s (%u,%u) -> Lv%u",
                 GetBuildingName(b->build_id), (unsigned)b->pos_x, (unsigned)b->pos_y,
                 (unsigned)(b->level + 1));
        ActivityLogPush(c, ACT_BUILD, ad);
    }
    c->automation.building_queue_active = true;
    c->automation.building_finish_time = 0;
    c->automation.building_pos_x = b->pos_x;
    c->automation.building_pos_y = b->pos_y;
    c->automation.building_last_action = now;
}

static uint8_t tech_level(Connection *c, uint16_t tech_id)
{
    if (tech_id == 0 || tech_id > 400) return 0;
    return GetTechLevel(c->technology.tech_data, tech_id);
}

static void AutoResearchTick(Connection *c)
{
    if (!c->automation.research || !c->automation.research_auto_start) return;
    if (c->automation.research_priority_count == 0 || !c->automation.research_loaded) return;
    uint32_t now = now32();
    if (now - c->automation.research_last_action < 3) return;
    /* After a 3203 rejection the server gets a pause before we retry, so a
       bad 3202 format does not hammer the server every 3s. */
    if (c->automation.research_retry_time && now < c->automation.research_retry_time) return;

    static uint32_t gate_log_at = 0;
    if (gate_log_at != now) {
        gate_log_at = now;
        LOGI("[RESEARCH GATE] loaded=%d tech=%u finish=%lld server=%llu auto_complete_free=%d prio=%u\n",
             c->automation.research_loaded, (unsigned)c->technology.research_tech,
             (long long)c->technology.finish_time, (unsigned long long)c->server_time,
             c->automation.research_auto_complete_free,
             (unsigned)c->automation.research_priority_count);
    }
    if (c->technology.research_tech != 0) {
        /* Research is in progress.  finish_time == 0 means we accepted a 3203
           but the authoritative 3201 RESEARCHINFO has not arrived yet — wait
           for it rather than complete-free a just-started research. */
        if (c->technology.finish_time == 0 ||
            c->technology.finish_time > (int64_t)c->server_time) return;
        uint16_t tid = c->technology.research_tech;
        if (c->automation.research_cancel_pending != 0) {
            uint16_t cid = c->automation.research_cancel_pending;
            if (cid != tid) {
                /* The active research changed under us; the pending cancel
                   no longer applies. */
                c->automation.research_cancel_pending = 0;
            } else {
                uint8_t clv = tech_level(c, cid);
                LOGI("[AUTO][RESEARCH] stuck research %u finished; cancelling via 3206\n",
                     (unsigned)cid);
                RequestResearchCancel(c, cid, clv);
                ActivityLogPush(c, ACT_RESEARCH_FREE, "Research Cancel: stuck active tech");
                c->automation.research_last_action = now;
                return;
            }
        }
        if (c->automation.research_auto_complete_free) {
            /* 3204 needs the active tech + target level (native: [tech_id][level+1]). */
            uint8_t lv = (tid != 0) ? tech_level(c, tid) : 0;
            RequestResearchCompleteFree(c, tid, lv);
            ActivityLogPush(c, ACT_RESEARCH_FREE, "Research Speed Up, New Time: Finished");
            c->automation.research_last_action = now;
            return;
        }
    }

    for (uint8_t i = 0; i < c->automation.research_priority_count; ++i) {
        uint16_t id = c->automation.research_priority[i];
        uint8_t lv = tech_level(c, id);
        if (lv >= c->automation.research_max_level) continue;
        const TechInfo *info = GetTechInfo(id);
        /* Cost gate (deadcode-skip): if this exact next step has a decoded
           cost and the account demonstrably cannot pay the food (and has no
           bag pack that would cover it), do NOT send the 3202 — it can only
           be rejected (3203 result=5) and just re-spams the server.  Try the
           next priority tech instead.  A lookup miss (unknown tech/level)
           forwards to the server as before, so nothing valid is ever blocked.
           The optional bag top-up below still runs first so a pay-worthy
           step that is merely short gets funded. */
        const GameResearchCost *gc = game_research_cost(id, lv + 1);
        /* Gate on ACTUAL stock in hand for the two scarce resources this step
           consumes: food and gold.  A bag pack does not count as "paid" — the
           top-up is a separate, later network request, so if we optimistically
           send 3202 the server will still reject result=5 and we have spammed
           it.  Only send when the account can genuinely cover the step now. */
        bool unaffordable = (gc != NULL) &&
                            (c->resources.food < gc->food ||
                             c->resources.gold < gc->gold);
        if (unaffordable) {
            LOGI("[AUTO][RESEARCH] skip %s id=%u Lv%u->%u: needs food=%u gold=%u, have food=%u gold=%u (unpayable) — deferring\n",
                 info ? info->name : "technology", id, lv, (unsigned)(lv + 1),
                 (unsigned)gc->food, (unsigned)gc->gold,
                 (unsigned)c->resources.food, (unsigned)c->resources.gold);
            c->automation.research_last_action = now; /* re-check next cycle, but cheap & offline */
            continue;
        }
        /* Prerequisite-tech gate (from TechLv.txt requires_tech1): a tech cannot
           be advanced to level N until its required tech is already at the same
           level N (e.g. tech 8 Gem Harvesting requires tech 7).  Send it later,
           after the prereq climbs. */
        if (gc != NULL && gc->req_tech != 0) {
            uint8_t pre = tech_level(c, gc->req_tech);
            if (pre < gc->req_tech_lv) {
                LOGI("[AUTO][RESEARCH] skip %s id=%u Lv%u->%u: needs tech %u at Lv%u, it is Lv%u (prereq) — deferring\n",
                     info ? info->name : "technology", id, lv, (unsigned)(lv + 1),
                     (unsigned)gc->req_tech, (unsigned)gc->req_tech_lv, (unsigned)pre);
                c->automation.research_last_action = now;
                continue;
            }
        }
        /* Best-effort bag top-up (never a gate): research may not actually be
           short — rss_floor is a guess, not the real cost the wire carries.
           Send 3202 and let the server accept or reject; the 3203 handler
           backs off so a genuinely-bad research cannot loop. */
        if (EnsureBuildResearchFloor(c)) {
            LOGI("[AUTO][RESEARCH] (bag top-up ok) %s id=%u level=%u -> %u\n",
                 info ? info->name : "technology", id, lv, (unsigned)(lv + 1));
        }
        LOGI("[AUTO][RESEARCH] starting %s id=%u level=%u -> %u\n",
             info ? info->name : "technology", id, lv, (unsigned)(lv + 1));
        RequestResearchStart(c, id, lv);
        {
            char ad[120];
            snprintf(ad, sizeof ad, "Research: %s -> Lv%u",
                     info ? info->name : "technology", (unsigned)(lv + 1));
            ActivityLogPush(c, ACT_RESEARCH, ad);
        }
        c->automation.research_last_action = now;
        return;
    }
}

/* Known resource pack items and how much of that resource each restores,
   one table per ResourceType. */
typedef struct {
    uint16_t id;
    uint32_t amount;
} ResourcePack;

static const ResourcePack kFoodPacks[] = {
    { FOOD_5K,      5000 },
    { FOOD_30K,     30000 },
    { FOOD_150K,    150000 },
    { FOOD_500K,    500000 },
    { FOOD_2M,      2000000 },
    { FOOD_6M,      6000000 },
    { FOOD_20M,     20000000 },
    { FOOD_60M,     60000000 },
};
static const ResourcePack kWoodPacks[] = { /* TIMBER_* */
    { TIMBER_3K,    3000 },
    { TIMBER_10K,   10000 },
    { TIMBER_50K,   50000 },
    { TIMBER_150K,  150000 },
    { TIMBER_500K,  500000 },
    { TIMBER_1_5M,  1500000 },
    { TIMBER_5M,    5000000 },
    { TIMBER_15M,   15000000 },
};
static const ResourcePack kRockPacks[] = { /* STONE_* */
    { STONE_3K,     3000 },
    { STONE_10K,    10000 },
    { STONE_50K,    50000 },
    { STONE_150K,   150000 },
    { STONE_500K,   500000 },
    { STONE_1_5M,   1500000 },
    { STONE_5M,     5000000 },
    { STONE_15M,    15000000 },
};
static const ResourcePack kOrePacks[] = { /* ORE_* */
    { ORE_3K,       3000 },
    { ORE_10K,      10000 },
    { ORE_50K,      50000 },
    { ORE_150K,     150000 },
    { ORE_500K,     500000 },
    { ORE_1_5M,     1500000 },
    { ORE_5M,       5000000 },
    { ORE_15M,      15000000 },
};
static const ResourcePack kGoldPacks[] = { /* GOLD_* */
    { GOLD_3K,      3000 },
    { GOLD_15K,     15000 },
    { GOLD_50K,     50000 },
    { GOLD_200K,    200000 },
    { GOLD_600K,    600000 },
    { GOLD_2M,      2000000 },
    { GOLD_6M,      6000000 },
};

/* Current on-hand balance of one resource type. */
static uint32_t ResourceBalance(Connection *c, ResourceType rt)
{
    switch (rt) {
        case RESOURCE_FOOD: return c->resources.food;
        case RESOURCE_WOOD: return c->resources.wood;
        case RESOURCE_ROCK: return c->resources.rock;
        case RESOURCE_ORE:  return c->resources.ore;
        case RESOURCE_GOLD: return c->resources.gold;
        default:            return 0;
    }
}

/* Pick a bag pack of the given resource for a needed amount. Prefer the
   smallest pack that covers the need (so we do not waste a big pack); if none
   covers it, fall back to the largest pack available (partial fill). Returns 0
   if the bag holds no pack of that resource. */
static uint16_t PickResourceItem(Connection *c, ResourceType rt, uint32_t need)
{
    const ResourcePack *packs;
    size_t n;
    switch (rt) {
        case RESOURCE_FOOD: packs = kFoodPacks; n = sizeof kFoodPacks / sizeof kFoodPacks[0]; break;
        case RESOURCE_WOOD: packs = kWoodPacks; n = sizeof kWoodPacks / sizeof kWoodPacks[0]; break;
        case RESOURCE_ROCK: packs = kRockPacks; n = sizeof kRockPacks / sizeof kRockPacks[0]; break;
        case RESOURCE_ORE:  packs = kOrePacks;  n = sizeof kOrePacks  / sizeof kOrePacks[0];  break;
        case RESOURCE_GOLD: packs = kGoldPacks; n = sizeof kGoldPacks / sizeof kGoldPacks[0]; break;
        default:            return 0;
    }

    uint16_t largest = 0;
    uint32_t largest_amount = 0;
    for (size_t i = 0; i < n; ++i) {
        const ResourcePack *p = &packs[i];
        if (c->items[p->id].quantity == 0)
            continue;
        /* source the restore amount from the decoded GameAssets Item table;
           fall back to the hardcoded value on a lookup miss so behavior never
           regresses. */
        uint32_t amt = game_item_pack_amount(p->id);
        if (amt == 0) amt = p->amount;
        if (amt >= need)
            return p->id;
        if (amt > largest_amount) {
            largest_amount = amt;
            largest = p->id;
        }
    }
    return largest;
}

/* Back-compat: food selection (used by the training food gate). */
static uint16_t PickFoodItem(Connection *c, uint32_t need)
{
    return PickResourceItem(c, RESOURCE_FOOD, need);
}

/* If the account's on-hand balance of `rt` is below `floor`, spend a matching
   bag pack to raise it. Rate-limited by *last_use so we never burn the bag
   faster than the server's resource update comes back. Logs what it spent.
   Returns true if the balance is at/above floor afterwards (or if the bag was
   simply empty and the caller should decide), false if it still can't be met. */
static bool TopUpResourceFromBag(Connection *c, ResourceType rt, uint32_t floor,
                                 uint32_t *last_use, uint32_t now)
{
    if (!c->items_loaded) return false;
    if (ResourceBalance(c, rt) >= floor) return true;

    if (now - *last_use < 20) return false;   /* rate-limit; retry next cycle */
    uint32_t need = floor - ResourceBalance(c, rt);
    uint16_t id = PickResourceItem(c, rt, need);
    if (id == 0) return false;                 /* no pack in the bag */

    RequestSimpleUseItem(c, id, 1);
    *last_use = now;
    LOGI("[BAG][TOPUP] %s %u -> need=%u floor=%u using item %u x1\n",
         rt == RESOURCE_FOOD ? "food" :
         rt == RESOURCE_WOOD ? "wood" :
         rt == RESOURCE_ROCK ? "rock" :
         rt == RESOURCE_ORE  ? "ore"  : "gold",
         ResourceBalance(c, rt), (unsigned)need, (unsigned)floor, (unsigned)id);
    {
        char ad[120];
        snprintf(ad, sizeof ad, "Using Bag %s Item %u x1 (top up)",
                 rt == RESOURCE_FOOD ? "Food" :
                 rt == RESOURCE_WOOD ? "Wood" :
                 rt == RESOURCE_ROCK ? "Rock" :
                 rt == RESOURCE_ORE  ? "Ore"  : "Gold", (unsigned)id);
        ActivityLogPush(c, ACT_ITEM_USE, ad);
    }
    return false; /* the balance update arrives async; re-check next cycle */
}

/* See forward declaration.  The gate's deficient resources are wood & rock. */
static bool BagCanStillBridge(Connection *c)
{
    uint32_t floor = c->automation.rss_floor;
    if (!c->automation.use_bag_rss || floor == 0) return false;
    if (ResourceBalance(c, RESOURCE_WOOD) < floor &&
        PickResourceItem(c, RESOURCE_WOOD, floor - ResourceBalance(c, RESOURCE_WOOD)) != 0)
        return true;
    if (ResourceBalance(c, RESOURCE_ROCK) < floor &&
        PickResourceItem(c, RESOURCE_ROCK, floor - ResourceBalance(c, RESOURCE_ROCK)) != 0)
        return true;
    return false;
}

/* use_bag_rss: before an expensive build/research send, top wood/rock up to
   rss_floor from the bag. Returns true when both are at floor (or the option
   is off), so the caller may proceed; false when one is stuck short (bag
   empty / rate-limited) and the request would just be rejected.

   Note: FOOD is deliberately NOT gated here.  Buildings and research consume
   wood/rock (+ occasionally ore/gold), not food; food packs on hand are also
   tiny (item #1014 = 150K) so forcing food to the floor stalls every build.
   Training already gates food separately in AutoTrainingTick. */
static bool EnsureBuildResearchFloor(Connection *c)
{
    if (!c->automation.use_bag_rss) return true;
    uint32_t now = now32();
    uint32_t floor = c->automation.rss_floor;
    if (floor == 0) return true;

    /* Anything already short and unfixable today blocks the spend.  Each
       resource has its own rate-limit slot so topping one up does not starve
       the other within the same cycle.
       FOOD is topped too (opportunistically): so far every build/research
       reject (2013 body=03 / 3203 result=5) has occurred with food~=0 in stock
       (confirmed real server state — a +30k food pack barely moved it 0->0.03).
       Army upkeep drains food to zero constantly.  If the near-max upgrades are
       gated on food, topping it can let them land; if not, the spend is simply
       wasted (food is cheap).  It stays opportunistic, never a hard gate, so an
       account with no food packs is not stalled. */
    TopUpResourceFromBag(c, RESOURCE_FOOD, floor,
                         &c->automation.build_bag_use_time[RESOURCE_FOOD], now);
    TopUpResourceFromBag(c, RESOURCE_WOOD, floor,
                         &c->automation.build_bag_use_time[RESOURCE_WOOD], now);
    TopUpResourceFromBag(c, RESOURCE_ROCK, floor,
                         &c->automation.build_bag_use_time[RESOURCE_ROCK], now);

    /* If either resource is below floor and could not be topped up, do not
       send the expensive request this cycle. */
    return ResourceBalance(c, RESOURCE_WOOD) >= floor &&
           ResourceBalance(c, RESOURCE_ROCK) >= floor;
}

static void AutoTrainingTick(Connection *c)
{
    if (!c->automation.training || !c->automation.training_auto_start) return;
    if (!c->troop.loaded) return;
    uint32_t now = now32();
    /* training_last_action doubles as a throttle (now) and as a reject
       backoff deadline (now+60, written by the 2408 handler).  A FUTURE
       deadline makes `now - training_last_action` wrap negative ~4.29e9,
       which is not < 5, so without this explicit future check the bot would
       instantly re-dispatch and hammer 2403 (258 requests in 150s observed).
       Guard the future case first, then fall through to the throttle. */
    if (c->automation.training_last_action > now) return;
    if (now - c->automation.training_last_action < 5) return;
    if (c->automation.training_queue_active) {
        /* finish_time stays 0 after an ACCEPT because we do not decode the
           batch's exact duration.  We rely on FINISHTRAINING (2413) to re-arm
           the queue, but that packet is NOT observed on this private server
           (neither 2409 nor 2413 appears in any capture).  A watchdog therefore
           force-re-arms once training_last_action (set to the ACCEPT time) is
           >4 minutes old, so the pump can never silently deadlock: the next
           2403 either starts the following batch or gets a harmless busy-reject.
           A duplicate 2403 while a batch runs is exactly result=1 — safe. */
        if (!c->automation.training_finish_time &&
            now - c->automation.training_last_action > 240) {
            c->automation.training_queue_active = false;
            c->automation.training_last_action = now;
            return;
        }
        if (c->automation.training_finish_time && c->automation.training_finish_time <= c->server_time) {
            RequestTrainingFinish(c);
            ActivityLogPush(c, ACT_TRAIN_SPEED, "Training Speed Up, New Time: Finished");
            c->automation.training_last_action = now;
        }
        return;
    }
    if (!c->resource_loaded) return;
    if (c->automation.training_kind > 3 || c->automation.training_tier > 3) return;
    if (c->automation.training_batch == 0) return;
    /* GameAssets Soldier gate (advisory, uses ONLY the validated tier field):
       confirm on the wire the configured tier actually exists in the decoded
       Soldier table for this troop kind before sending 2403.  A lookup miss
       (= GameAssets does not know this tier/kind) is a configuration problem,
       not a server state — skip silently rather than blindly sending a tier the
       decoded data cannot confirm.  Mirrors the cost gates: cannot regress the
       adaptive highest-tier reject logic.  Log the table's load state once so a
       live run visibly proves the GameAssets info is compiled in and running. */
    {
        static int gaconf = 0;
        if (!gaconf) {
            unsigned cnt = 0, maxt = 0;
            for (unsigned i = 0; i < GAME_SOLDIER_N; ++i)
                if (kGameSoldiers[i].tier) { ++cnt; if (kGameSoldiers[i].tier > maxt) maxt = kGameSoldiers[i].tier; }
            LOGI("[AUTO][GAMEASSETS] Soldier table live: %u validated soldiers, tiers 1..%u\n",
                 cnt, maxt);
            gaconf = 1;
        }
        /* Soldier id for this kind/tier = config_tier(0-based)*4 + kind + 1. */
        uint16_t sid = (uint16_t)(c->automation.training_tier * 4 + c->automation.training_kind + 1);
        const GameSoldier *gs = game_soldier_get(sid);
        if (!gs || gs->tier != (uint16_t)(c->automation.training_tier + 1)) {
            LOGI("[AUTO][TRAIN] GameAssets has no Soldier for kind=%u tier=%u (soldier id %u); skipping\n",
                 c->automation.training_kind, c->automation.training_tier, sid);
            return;
        }
    }
    /* Resource gate: training consumes food (and other resources). If the
       stock cannot cover even one batch, do not hammer 2403 every 5s — the
       server would just reject. Retry once resources recover. */
    if (c->resources.food < c->automation.training_batch) {
        /* Bag fallback (use_bag_rss): when the stock is starved (e.g. army
           upkeep eats all food), consume a food pack from the bag so training
           can proceed. Rate-limited so we never burn the bag faster than the
           server's resource update comes back. */
        if (c->automation.use_bag_rss && c->items_loaded &&
            now - c->automation.training_bag_use_time >= 20) {
            uint16_t fid = PickFoodItem(c, c->automation.training_batch);
            if (fid) {
                LOGI("[AUTO][TRAIN] food %u < batch %u; using bag food item %u\n",
                     (unsigned)c->resources.food, (unsigned)c->automation.training_batch, fid);
                RequestSimpleUseItem(c, fid, 1);
                {
                    char ad[120];
                    snprintf(ad, sizeof ad, "Using Bag Food Item %u x1 (feed stock)", (unsigned)fid);
                    ActivityLogPush(c, ACT_ITEM_USE, ad);
                }
                c->automation.training_bag_use_time = now;
                c->automation.training_last_action = now;
                return;
            }
        }
        if (now % 30 == 0) /* rate-limit the diagnostic */
            LOGI("[AUTO][TRAIN] insufficient food %u < batch %u; waiting for resources\n",
                 (unsigned)c->resources.food, (unsigned)c->automation.training_batch);
        return;
    }

    LOGI("[AUTO][TRAIN] kind=%u tier=%u amount=%u\n",
         c->automation.training_kind, c->automation.training_tier,
         c->automation.training_batch);
    /* The game's TroopType enum is 0-based (Infantry=0, Ranged=1, Cavalry=2,
       Siege=3) and RequestTroopTraining pipes the kind through
       SmartUseForTrainingProtocol(byte TroopType, ...) verbatim, so the wire
       kind is 0-based: send the config value as-is, no +1. */
    RequestTroopTraining(c, c->automation.training_kind,
                         c->automation.training_tier,
                         c->automation.training_batch);
    {
        char ad[120];
        snprintf(ad, sizeof ad, "Training Troops: %s x %u",
                 troop_name(c->automation.training_kind),
                 (unsigned)c->automation.training_batch);
        ActivityLogPush(c, ACT_TRAIN, ad);
    }
    c->automation.training_queue_active = true;
    c->automation.training_last_action = now;
}

static void AutoHospitalTick(Connection *c)
{
    if (!c->automation.hospital || !c->automation.hospital_auto_heal) return;
    if (!c->wounded.loaded || c->wounded.troop.total == 0) return;
    uint32_t now = now32();
    if (now - c->automation.hospital_last_action < 10) return;
    if (c->automation.hospital_queue_active) return;

    LOGI("[AUTO][HOSPITAL] healing %u wounded troops\n", c->wounded.troop.total);
    RequestHealing(c, c->automation.hospital_instant_heal);
    {
        char ad[120];
        snprintf(ad, sizeof ad, "Healing %u wounded troops",
                 (unsigned)c->wounded.troop.total);
        ActivityLogPush(c, ACT_HEAL, ad);
    }
    c->automation.hospital_queue_active = true;
    c->automation.hospital_last_action = now;
}

static void AutoDailyMissionTick(Connection *c)
{
    if (!c->automation.daily_mission) return;
    if (!c->daily_mission.loaded) {
        /* Request daily mission info - packet 11871 */
        RequestDailyMissionInfo(c);
        return;
    }
    uint32_t now = now32();
    if (now - c->automation.daily_mission_last_action < 10) return;

    /* Check for claimable rewards */
    if (c->daily_mission.total_points > c->daily_mission.claimed_points) {
        for (uint8_t i = 0; i < c->daily_mission.reward_stages_count; ++i) {
            if (c->daily_mission.reward_stage_ids[i] &&
                !c->daily_mission.reward_stage_claimed[i] &&
                c->daily_mission.total_points >= c->daily_mission.reward_stage_ids[i]) {
                RequestDailyMissionReward(c, c->daily_mission.reward_stage_ids[i]);
                ActivityLogPush(c, ACT_CLAIM, "Collecting Daily Mission stage reward");
                c->automation.daily_mission_last_action = now;
                return;
            }
        }
    }

    /* Check for completed unclaimed missions */
    for (uint32_t i = 0; i < c->daily_mission.missions_count; ++i) {
        if (c->daily_mission.missions[i].completed &&
            !c->daily_mission.missions[i].claimed) {
            RequestDailyMissionReward(c, c->daily_mission.missions[i].mission_id);
            ActivityLogPush(c, ACT_CLAIM, "Collecting Daily Mission quest reward");
            c->automation.daily_mission_last_action = now;
            return;
        }
    }
}

static void AutoVipMissionTick(Connection *c)
{
    if (!c->automation.vip_mission) return;
    if (!c->vip_mission.loaded) {
        RequestVipMissionInfo(c);
        return;
    }
    uint32_t now = now32();
    if (now - c->automation.vip_mission_last_action < 10) return;
    /* Honor the per-action key: master switch alone must not cause a collect. */
    if (!c->automation.vip_mission_auto_collect) return;

    for (uint32_t i = 0; i < c->vip_mission.missions_count; ++i) {
        if (c->vip_mission.missions[i].completed &&
            !c->vip_mission.missions[i].claimed) {
            RequestVipMissionCollect(c, c->vip_mission.missions[i].mission_id);
            ActivityLogPush(c, ACT_CLAIM, "Collecting VIP Mission reward");
            c->automation.vip_mission_last_action = now;
            return;
        }
    }
}

static void AutoDailySigninTick(Connection *c)
{
    if (!c->automation.daily_signin) return;
    if (!c->daily_signin.loaded) {
        RequestDailySigninInfo(c);
        return;
    }
    uint32_t now = now32();
    if (now - c->automation.daily_signin_last_action < 60) return; /* Check once per minute */
    /* Honor the per-action key: master switch alone must not cause a sign-in. */
    if (!c->automation.daily_signin_auto_signin) return;

    if (!c->daily_signin.signed_today) {
        RequestDailySignin(c);
        c->automation.daily_signin_last_action = now;
    }
}

static void AutoPetTrainingTick(Connection *c)
{
    if (!c->automation.pet_training) return;
    if (!c->pet_training.loaded) {
        RequestPetTrainingInfo(c);
        return;
    }
    uint32_t now = now32();
    if (now - c->automation.pet_training_last_action < 10) return;

    /* Check for finished training slots */
    if (c->automation.pet_training_auto_complete_free) {
        for (uint8_t i = 0; i < c->pet_training.training_slots_count; ++i) {
            if (c->pet_training.slots[i].active &&
                c->pet_training.slots[i].start_time + c->pet_training.slots[i].duration <= (uint64_t)c->server_time) {
                RequestPetTrainingFinish(c, c->pet_training.slots[i].slot_index);
                c->automation.pet_training_last_action = now;
                return;
            }
        }
    }

    /* Start new training if slots available and pets configured (per-action gate) */
    if (c->automation.pet_training_auto_start) {
        for (uint8_t i = 0; i < c->pet_training.training_slots_count; ++i) {
            if (!c->pet_training.slots[i].active &&
                c->automation.pet_training_pet_id[i] &&
                c->automation.pet_training_type[i]) {
                RequestPetTrainingBegin(c,
                    c->automation.pet_training_pet_id[i],
                    c->automation.pet_training_type[i],
                    c->automation.pet_training_use_speedup ||
                    c->automation.pet_training_auto_instant);
                c->automation.pet_training_last_action = now;
                return;
            }
        }
    }
}

static void AutoHeroTick(Connection *c)
{
    if (!c->automation.hero_system) return;
    if (!c->hero.loaded) {
        RequestHeroInfo(c);
        return;
    }
    uint32_t now = now32();
    if (now - c->automation.hero_last_action < 30) return;

    /* Check for finished hero enhancements (per-action gate) */
    if (c->automation.hero_auto_enhance) {
        for (uint16_t i = 0; i < c->hero.heroes_count; ++i) {
            if (c->hero.heroes[i].enhancement_active &&
                c->hero.heroes[i].enhancement_finish_time <= (uint64_t)c->server_time) {
                RequestHeroEnhanceFinish(c, c->hero.heroes[i].hero_id);
                c->automation.hero_last_action = now;
                return;
            }
        }
    }
    /* Hero star-up (hero_auto_starup) and hero skill (hero_auto_skill) are gated
       off by design: NO RequestHeroStarup / RequestHeroSkill SEND wrapper exists
       in this codebase (receive handlers only). Per the IL2CPP ground-truth rule
       we must NOT fabricate the wire. Wire these against a libil2cpp capture of
       the request before enabling; until then nothing dispatches even if the
       config key is flipped. */
    if (c->automation.hero_auto_starup) {
        /* TODO(hero-wire): RequestHeroStarup(c, hero_id) -- request layout
           unvalidated; see libil2cpp.so StarUpEvent send path. */
    }
    if (c->automation.hero_auto_skill) {
        /* TODO(hero-wire): RequestHeroSkill(c, hero_id, skill_slot) -- request
           layout unvalidated; see libil2cpp.so SkillLevelUp send path. */
    }
}

static void AutoItemCraftTick(Connection *c)
{
    if (!c->automation.item_craft) return;
    if (!c->item_craft.loaded) {
        RequestItemCraftInfo(c);
        return;
    }
    uint32_t now = now32();
    if (now - c->automation.item_craft_last_action < 10) return;

    /* Check for finished crafting (per-action gate) */
    if (c->automation.item_craft_auto_finish) {
        for (uint16_t i = 0; i < c->item_craft.recipes_count; ++i) {
            if (c->item_craft.recipes[i].crafting_active &&
                c->item_craft.recipes[i].finish_time <= (uint64_t)c->server_time) {
                RequestItemCraftFinish(c, c->item_craft.recipes[i].recipe_id);
                c->automation.item_craft_last_action = now;
                return;
            }
        }
    }

    /* Start new craft if recipes configured and materials available (per-action gate) */
    if (c->automation.item_craft_auto_start) {
        for (uint16_t i = 0; i < c->item_craft.recipes_count; ++i) {
            if (!c->item_craft.recipes[i].crafting_active &&
                c->automation.item_craft_recipe_id[i] &&
                c->automation.item_craft_count[i] > 0) {
                RequestItemCraftStart(c, c->automation.item_craft_recipe_id[i],
                                      c->automation.item_craft_count[i]);
                c->automation.item_craft_last_action = now;
                return;
            }
        }
    }
}

static void AutoAchievementTick(Connection *c)
{
    if (!c->automation.achievement) return;
    if (!c->achievement.loaded) {
        uint32_t now = now32();
        /* The private server never answers RequestAchievementInfo (it only pushes a
         * login snapshot under type 3181, which is not the real activity-array layout
         * so it stays un-loaded). Throttle or this floods every AutomationTick. */
        if (now - c->automation.achievement_last_action < 60) return;
        c->automation.achievement_last_action = now;
        RequestAchievementInfo(c);
        return;
    }
    uint32_t now = now32();
    if (now - c->automation.achievement_last_action < 30) return;

    /* The private server broadcasts an ad-hoc login snapshot (not the real-client
     * layout); overreading it fabricates bogus activities (activity_id=0,
     * reward_id=255). Never claim an activity/reward we did not truly parse. */
    if (c->achievement.activities_count == 0) return;

    for (uint32_t i = 0; i < c->achievement.activities_count && i < 16; ++i) {
        if (c->achievement.activities[i].activity_id == 0) continue;
        if (c->achievement.activities[i].rewards_count == 0 ||
            c->achievement.activities[i].rewards_count > 8) continue;
        for (uint8_t j = 0; j < c->achievement.activities[i].rewards_count; ++j) {
            if (c->achievement.activities[i].reward_ids[j] == 0 ||
                c->achievement.activities[i].reward_ids[j] == 0xFF) continue;
            if (c->achievement.activities[i].current_points >=
                c->achievement.activities[i].target_points &&
                !c->achievement.activities[i].reward_claimed[j]) {
                /* 3179 is polymorphic -- echo the per-activity achievement_kind
                   the server reported (RecvAchievementInfo), unless a fixed
                   override kind is configured for A/B testing.  Never send
                   kind=0 (invalid discriminator): default to SoloBattle 0x12. */
                uint8_t ach_kind = c->automation.achievement_kind_override
                    ? c->automation.achievement_kind_override
                    : c->achievement.activities[i].achievement_kind;
                if (ach_kind == 0) ach_kind = 0x12;
                RequestAchievementPrize(c, ach_kind,
                    c->achievement.activities[i].activity_id,
                    c->achievement.activities[i].reward_ids[j]);
                ActivityLogPush(c, ACT_CLAIM, "Collecting Achievement prize");
                c->automation.achievement_last_action = now;
                return;
            }
        }
    }
}

static void AutoQuestChapterTick(Connection *c)
{
    if (!c->automation.quest_chapter) return;
    if (!c->quest_chapter.loaded) {
        uint32_t now = now32();
        if (now - c->automation.quest_chapter_last_action < 60) return;
        c->automation.quest_chapter_last_action = now;
        RequestQuestChapterInfo(c);
        return;
    }
    uint32_t now = now32();
    if (now - c->automation.quest_chapter_last_action < 30) return;

    for (uint32_t i = 0; i < c->quest_chapter.quests_count; ++i) {
        if (c->quest_chapter.quests[i].completed &&
            !c->quest_chapter.quests[i].claimed) {
            RequestQuestChapterReward(c, c->quest_chapter.quests[i].quest_id);
            ActivityLogPush(c, ACT_CLAIM, "Collecting Quest Chapter reward");
            c->automation.quest_chapter_last_action = now;
            return;
        }
    }
}

/* =========================================================================
 * NEW SUBSYSTEM AUTOMATION TICKS
 * These functions implement automation for subsystems discovered in the dump
 * ========================================================================= */

/* ---- Expedition ---- */
static void AutoExpeditionTick(Connection *c)
{
    if (!c->automation.expedition) return;
    if (!c->expedition.info_loaded) {
        RequestExpeditionInfo(c);
        return;
    }
    uint32_t now = now32();
    if (now - c->automation.expedition_last_action < 30) return;

    if (c->expedition.unlocked && !c->expedition.prize_claimed) {
        RequestExpeditionPrize(c, c->expedition.expedition_id);
        ActivityLogPush(c, ACT_CLAIM, "Collecting Expedition prize");
        c->automation.expedition_last_action = now;
        return;
    }
}

/* ---- Valhalla ---- */
static void AutoValhallaTick(Connection *c)
{
    if (!c->automation.valhalla) return;
    if (!c->valhalla.info_loaded) {
        RequestValhallaInfo(c);
        return;
    }
    uint32_t now = now32();
    if (now - c->automation.valhalla_last_action < 60) return; /* Check once per minute */

    /* Check for claimable prizes */
    if (c->valhalla.prize_available) {
        RequestValhallaPrize(c);
        ActivityLogPush(c, ACT_CLAIM, "Collecting Valhalla prize");
        c->automation.valhalla_last_action = now;
        return;
    }
}

/* ---- Adventure ---- */
static void AutoAdventureTick(Connection *c)
{
    if (!c->automation.adventure) return;
    if (!c->adventure.loaded) {
        RequestAdventureMissionInfo(c);
        return;
    }
    uint32_t now = now32();
    if (now - c->automation.adventure_last_action < 30) return;

    /* Check for completed missions with unclaimed rewards */
    if (c->adventure.completed && !c->adventure.prize_claimed) {
        RequestAdventureMissionPrize(c, c->adventure.mission_id);
        ActivityLogPush(c, ACT_CLAIM, "Collecting Adventure mission prize");
        c->automation.adventure_last_action = now;
        return;
    }
}

/* ---- Relics ---- */
static void AutoRelicsTick(Connection *c)
{
    if (!c->automation.relics) return;
    if (!c->relics.info_loaded) {
        RequestRelicsInfo(c);
        return;
    }
    uint32_t now = now32();
    if (now - c->automation.relics_last_action < 60) return; /* Check once per minute */

    /* Each action is gated on its own per-action key (all default false). Relic
       gacha/synthesize/enhance spend currency/relic shards, so nothing fires
       until a user flips the relevant switch. Targets come from the parsed info
       state (gacha_list / relics), selected most-significant-first. */
    if (c->automation.relics_auto_gacha && c->relics.gacha_list_count > 0) {
        RequestRelicsGacha(c, c->relics.gacha_list[0].gacha_id);
        ActivityLogPush(c, ACT_CLAIM, "Relics: gacha spin");
        c->automation.relics_last_action = now;
        return;
    }
    if (c->automation.relics_auto_synthesize && c->relics.relics_count > 0) {
        RequestRelicsSynthesize(c, c->relics.relics[0].relic_id);
        ActivityLogPush(c, ACT_CLAIM, "Relics: synthesize");
        c->automation.relics_last_action = now;
        return;
    }
    if (c->automation.relics_auto_enhance && c->relics.relics_count > 0) {
        RequestRelicsEnhance(c, c->relics.relics[0].relic_id);
        ActivityLogPush(c, ACT_CLAIM, "Relics: enhance");
        c->automation.relics_last_action = now;
        return;
    }
}

/* ---- Serial Gift ---- */
static void AutoSerialGiftTick(Connection *c)
{
    if (!c->automation.serial_gift) return;
    if (!c->serial_gift.list_loaded) {
        uint32_t now = now32();
        if (now - c->automation.serial_gift_last_action < 60) return;
        c->automation.serial_gift_last_action = now;
        RequestSerialGiftList(c);
        return;
    }
    uint32_t now = now32();
    if (now - c->automation.serial_gift_last_action < 300) return; /* Check every 5 minutes */

    /* Check for claimable gifts */
    for (uint16_t i = 0; i < c->serial_gift.gifts_count; ++i) {
        if (c->serial_gift.gifts[i].available &&
            !c->serial_gift.gifts[i].claimed) {
            /* SEND_REQUEST_SERIALGIFT_GETGIFT takes uint8_t stage = the gift's
               ordinal index within the event list (i), not its item id. */
            RequestSerialGiftGet(c, (uint8_t)i);
            ActivityLogPush(c, ACT_CLAIM, "Collecting Serial Gift");
            c->automation.serial_gift_last_action = now;
            return;
        }
    }
}

/* ---- Old Player Back (claim-only) ---- */
/* The server pushes RESP_OLDPLAYERBACK_INFO; there is no request cmd for it,
   so we only act when the pushed state marks a gift as available. Claiming is
   a header-only request with no purchase / welcome-flag / crossover side path. */
static void AutoOldPlayerBackTick(Connection *c)
{
    if (!c->automation.oldplayerback) return;
    if (!c->oldplayerback.loaded) return; /* wait for server push */
    uint32_t now = now32();
    if (now - c->automation.oldplayerback_last_action < 300) return;

    if (c->oldplayerback.gift_available && !c->oldplayerback.gift_claimed) {
        RequestOldPlayerBackGetGift(c);
        ActivityLogPush(c, ACT_CLAIM, "Collecting Old Player Back gift");
        c->automation.oldplayerback_last_action = now;
        return;
    }
}

/* ---- Gift Activity (claim-only) ---- */
/* Free gift-box opening only; the random-unlock / spend path is not called. */
static void AutoGiftActivityTick(Connection *c)
{
    if (!c->automation.gift_activity) return;
    if (!c->gift_activity.loaded) {
        uint32_t now = now32();
        if (now - c->automation.gift_activity_last_action < 60) return;
        c->automation.gift_activity_last_action = now;
        RequestGiftActivityList(c);
        return;
    }
    uint32_t now = now32();
    if (now - c->automation.gift_activity_last_action < 300) return;

    for (uint16_t i = 0; i < c->gift_activity.box_count; ++i) {
        if (c->gift_activity.boxes[i].available &&
            !c->gift_activity.boxes[i].claimed) {
            /* OPEN_GIFT_BOX takes byte boxIdx = the box's ordinal index (i),
               not its item id — matches RecvGiftActivityOpenBox's idx echo. */
            RequestGiftActivityOpenBox(c, (uint8_t)i);
            ActivityLogPush(c, ACT_CLAIM, "Opening Gift Activity box");
            c->automation.gift_activity_last_action = now;
            return;
        }
    }
}

/* ---- Week Challenge ---- */
static void AutoWeekChallengeTick(Connection *c)
{
    if (!c->automation.week_challenge) return;
    if (!c->week_challenge.info_loaded) {
        RequestWeekChallengeInfo(c);
        return;
    }
    uint32_t now = now32();
    if (now - c->automation.week_challenge_last_action < 60) return; /* Check once per minute */

    /* Check for claimable prizes */
    if (c->week_challenge.prize_loaded && c->week_challenge.prize_available) {
        RequestWeekChallengePrize(c);
        ActivityLogPush(c, ACT_CLAIM, "Collecting Week Challenge prize");
        c->automation.week_challenge_last_action = now;
        return;
    }
}

/* ---- Lucky Card ---- */
static void AutoLuckyCardTick(Connection *c)
{
    if (!c->automation.lucky_card) return;
    if (!c->lucky_card.info_loaded) {
        RequestLuckyCardInfo(c);
        return;
    }
    uint32_t now = now32();
    if (now - c->automation.lucky_card_last_action < 300) return; /* Check every 5 minutes */
    /* Per-action gate: master switch alone must not exchange cards. */
    if (!c->automation.lucky_card_auto_exchange) return;

    /* Exchange the first unclaimed prize card (spends currency; off by default). */
    for (uint16_t i = 0; i < c->lucky_card.prizes_count && i < 16; ++i) {
        if (!c->lucky_card.prizes[i].claimed) {
            RequestLuckyCardExchange(c, c->lucky_card.prizes[i].card_index);
            ActivityLogPush(c, ACT_CLAIM, "Lucky Card: exchange");
            c->automation.lucky_card_last_action = now;
            return;
        }
    }
}

/* ---- Dark Nest ---- */
static void AutoDarkNestTick(Connection *c)
{
    if (!c->automation.dark_nest) return;
    if (!c->dark_nest.rally_list_loaded) {
        RequestDarkNestRallyList(c);
        return;
    }
    uint32_t now = now32();
    if (now - c->automation.dark_nest_last_action < 60) return; /* Check once per minute */
    /* Per-action gate: joining a rally commits troops — off by default. */
    if (!c->automation.dark_nest_auto_rally) return;

    for (uint16_t i = 0; i < c->dark_nest.rally_count && i < 30; ++i) {
        uint32_t troops[16];
        if (build_troop_array(c, troops)) {
            RequestDarkNestJoinRally(c, c->dark_nest.rallies[i].rally_id, troops);
            ActivityLogPush(c, ACT_CLAIM, "Dark Nest: join rally");
            c->automation.dark_nest_last_action = now;
            return;
        }
    }
}

/* ---- Fantasy Realm ---- */
static void AutoFantasyRealmTick(Connection *c)
{
    if (!c->automation.fantasy_realm) return;
    if (!c->fantasy_realm.loaded) {
        RequestFantasyRealmInfo(c);
        return;
    }
    uint32_t now = now32();
    if (now - c->automation.fantasy_realm_last_action < 30) return;

    /* Check for available monsters to hunt */
    if (c->automation.fantasy_realm_auto_hunt && c->fantasy_realm.monsters_count > 0) {
        for (uint8_t i = 0; i < c->fantasy_realm.monsters_count && i < 32; ++i) {
            if (!c->fantasy_realm.monsters[i].hunted) {
                RequestFantasyRealmHunt(c, c->fantasy_realm.monsters[i].monster_id);
                c->automation.fantasy_realm_last_action = now;
                return;
            }
        }
    }
}

/* ---- Growth Fund ---- */
static void AutoGrowthFundTick(Connection *c)
{
    if (!c->automation.growth_fund) return;
    if (!c->growth_fund.info_loaded) {
        RequestGrowthFundInfo(c);
        return;
    }
    uint32_t now = now32();
    if (now - c->automation.growth_fund_last_action < 300) return; /* Check every 5 minutes */

    /* Check for claimable prizes based on lord level */
    if (c->growth_fund.prize_available && c->player.lord_level >= c->growth_fund.required_level) {
        RequestGrowthFundPrize(c, c->growth_fund.required_level);
        ActivityLogPush(c, ACT_CLAIM, "Collecting Growth Fund prize");
        c->automation.growth_fund_last_action = now;
        return;
    }
}

/* ---- Treasure Back Event ---- */
static void AutoTreasureBackTick(Connection *c)
{
    if (!c->automation.treasure_back_event) return;
    if (!c->treasure_back_event.info_loaded) {
        RequestTreasureBackEventInfo(c);
        return;
    }
    uint32_t now = now32();
    if (now - c->automation.treasure_back_event_last_action < 300) return; /* Check every 5 minutes */

    /* Check for claimable prizes */
    for (uint16_t i = 0; i < c->treasure_back_event.prizes_count; ++i) {
        if (c->treasure_back_event.prizes[i].available &&
            !c->treasure_back_event.prizes[i].claimed &&
            c->treasure_back_event.points >= c->treasure_back_event.prizes[i].required_points) {
            RequestTreasureBackEventPrize(c, c->treasure_back_event.prizes[i].prize_id);
            ActivityLogPush(c, ACT_CLAIM, "Collecting Treasure Back Event prize");
            c->automation.treasure_back_event_last_action = now;
            return;
        }
    }
}

/* ---- Newbie Challenge ---- */
static void AutoNewbieChallengeTick(Connection *c)
{
    if (!c->automation.newbie_challenge) return;
    if (!c->newbie_challenge.info_loaded) {
        RequestNewbieChallengeInfo(c);
        return;
    }
    uint32_t now = now32();
    if (now - c->automation.newbie_challenge_last_action < 300) return; /* Check every 5 minutes */

    /* Check for claimable prizes */
    if (c->newbie_challenge.prize_available) {
        RequestNewbieChallengePrize(c, c->newbie_challenge.next_prize_id, c->newbie_challenge.is_vip);
        c->automation.newbie_challenge_last_action = now;
        return;
    }
}

/* ---- Cycle Mission ---- */
static void AutoCycleMissionTick(Connection *c)
{
    if (!c->automation.cycle_mission) return;
    if (!c->cycle_mission.info_loaded) {
        RequestCycleMissionInfo(c);
        return;
    }
    uint32_t now = now32();
    if (now - c->automation.cycle_mission_last_action < 300) return; /* Check every 5 minutes */

    /* Check for claimable prizes */
    if (c->cycle_mission.prize_available) {
        RequestCycleMissionPrize(c, c->cycle_mission.combo_id);
        c->automation.cycle_mission_last_action = now;
        return;
    }
}

/* ---- Custom Mission ---- */
static void AutoCustomMissionTick(Connection *c)
{
    if (!c->automation.custom_mission) return;
    if (!c->custom_mission.info_loaded) {
        RequestCustomMissionInfo(c);
        return;
    }
    uint32_t now = now32();
    if (now - c->automation.custom_mission_last_action < 300) return; /* Check every 5 minutes */

    /* Check for claimable prizes */
    for (uint16_t i = 0; i < c->custom_mission.mission_count; ++i) {
        if (c->custom_mission.missions[i].completed &&
            !c->custom_mission.missions[i].prize_claimed) {
            RequestCustomMissionPrize(c, c->custom_mission.missions[i].mission_id);
            c->automation.custom_mission_last_action = now;
            return;
        }
    }
}

/* ---- Mobilization (Alliance Mobilization) ---- */
static void AutoMobilizationTick(Connection *c)
{
    if (!c->automation.mobilization) return;
    if (!c->mobilization.mission_loaded) {
        RequestMobilizationMissionData(c);
        return;
    }
    uint32_t now = now32();
    if (now - c->automation.mobilization_last_action < 60) return; /* Check once per minute */

    /* Claim personal degree prizes */
    if (c->automation.mobilization_auto_claim) {
        if (c->mobilization.personal_score >= c->mobilization.personal_last_stage_point &&
            c->mobilization.personal_prize_earned_step < c->mobilization.personal_extra_prize_earned_times + 1) {
            RequestActivityAmGetPersonalPrize(c);
            c->automation.mobilization_last_action = now;
            return;
        }
    }

    /* Refresh available missions */
    if (c->automation.mobilization_auto_refresh &&
        c->mobilization.available_mission > 0 &&
        c->mobilization.available_mission_cd_time <= (int64_t)c->server_time) {
        RequestMobilizationMissionRefresh(c, 0, 0, 0);
        c->automation.mobilization_last_action = now;
        return;
    }

    /* Buy extra missions if available */
    if (c->automation.mobilization_auto_buy &&
        c->mobilization.extra_mission > 0 &&
        c->mobilization.involved_member < 5) {
        RequestMobilizationMissionBuy(c);
        c->automation.mobilization_last_action = now;
        return;
    }
}

/* ---- Alliance Gather Point ---- */
static void AutoAllianceGatherPointTick(Connection *c)
{
    if (!c->automation.alliance_gather_point) return;
    if (!c->alliance_gather_point.loaded) {
        RequestAllianceGatherPointInfo(c);
        return;
    }
    uint32_t now = now32();
    if (now - c->automation.alliance_gather_point_last_action < 300) return; /* Check every 5 minutes */

    /* Auto teleport to gather point if enabled */
    if (c->automation.alliance_gather_point_auto_teleport &&
        c->alliance_gather_point.point_set) {
        // Check if we need to teleport
        // This would require checking current position vs gather point
        // For now, just refresh info periodically
        RequestAllianceGatherPointInfo(c);
        c->automation.alliance_gather_point_last_action = now;
        return;
    }
}

/* ---- Alliance WhiteList ---- */
static void AutoAllianceWhiteListTick(Connection *c)
{
    if (!c->automation.alliance_whitelist) return;
    if (!c->alliance_whitelist.loaded) {
        RequestAllianceWhiteListInfo(c);
        return;
    }
    uint32_t now = now32();
    if (now - c->automation.alliance_whitelist_last_action < 300) return; /* Check every 5 minutes */

    /* Auto-accept is typically a manual configuration - just keep list refreshed */
    if (c->automation.alliance_whitelist_auto_accept) {
        RequestAllianceWhiteListInfo(c);
        c->automation.alliance_whitelist_last_action = now;
    }
}

void AutomationTick(Connection *c)
{
    if (!c || c->server_time == 0 || !c->automation.enabled) return;

    /* Existing verified paths. */
    if (c->automation.economy) BlackMarketTick(c);
    if (c->automation.alliance) AllianceGiftTick(c);
    if (c->automation.map) ResourceTransferTick(c);

    /* Queue-driven domains reconstructed from the client declarations. */
    AutoBuildingTick(c);
    AutoResearchTick(c);
    AutoTrainingTick(c);
    AutoHospitalTick(c);

    /* New automation subsystems from dump */
    AutoDailyMissionTick(c);
    AutoVipMissionTick(c);
    AutoDailySigninTick(c);
    AutoPetTrainingTick(c);
    AutoHeroTick(c);
    AutoItemCraftTick(c);
    AutoAchievementTick(c);
    AutoQuestChapterTick(c);

    /* Additional automation subsystems from dump (newly implemented) */
    AutoExpeditionTick(c);
    AutoValhallaTick(c);
    AutoAdventureTick(c);
    AutoRelicsTick(c);
    AutoSerialGiftTick(c);
    AutoOldPlayerBackTick(c);
    AutoGiftActivityTick(c);
    AutoWeekChallengeTick(c);
    AutoLuckyCardTick(c);
    AutoDarkNestTick(c);
    AutoFantasyRealmTick(c);
    AutoGrowthFundTick(c);
    AutoTreasureBackTick(c);
    AutoNewbieChallengeTick(c);
    AutoCycleMissionTick(c);
    AutoCustomMissionTick(c);

    /* Alliance mobilization & management */
    AutoMobilizationTick(c);
    AutoAllianceGatherPointTick(c);
    AutoAllianceWhiteListTick(c);

    /* Protection/gathering are existing independent engines. */
    ShieldTick(c);
    if (c->automation.war) {
        LOGI("[AUTO][WAR] enabled by configuration; no automatic war action is dispatched by this build.\n");
    }
    GatheringTick(c);
}
