#include "connection.h"
#include "protocol.h"
#include "map_point.h"
#include "net_rw.h"
#include "MAP_UPDATE_KIND.h"
#include "packet_enum.h"
#include "log.h"
#include "activity_log.h"
#include <string.h>
#include <stdio.h>
#include <math.h>
#include <time.h>

/* TEMPORARY: capture full 2220 (MAPINFO_PLUS) packets to map_2220_*.txt so the
   real resource-record framing can be decoded from ground truth rather than a
   guessed layout.  Remove once the parser is driven from an actual capture. */
#define GATHER_DEBUG_DUMP 1

/* After this many consecutive 6615 acknowledgements that never became a
   dispatched gather march (server classified them Standby, current_marches
   stayed 0), suspend gathering for a backoff window. */
#define GATHER_STANDBY_REJECT_LIMIT     4
#define GATHER_STANDBY_SUSPEND_SECONDS  300

/* Upper/lower bound for a single gather march body (task #8).  Marches must be
   carry-sized, not the whole army: a level-4 food tile holds ~1.57M, so more
   than a few thousand troops is surplus that typifies a fabricated request. */
#define GATHER_MARCH_TROOP_CAP          6000u
#define GATHER_MARCH_TROOP_MIN          1u

static uint32_t now_sec(void) { return (uint32_t)time(NULL); }
static bool IsInSchedule(const GatheringState *g, uint32_t now);

static int resource_enabled(const GatheringState *g, uint8_t kind) {
    if (kind >= 1 && kind <= 6) return g->resource_enabled[kind];
    return 0;
}

static int level_enabled(const GatheringState *g, uint8_t level) {
    if (level >= 1 && level <= 5) return g->level_enabled[level];
    if (level == 6) return g->gems_ignore_level || g->level_enabled[5];  // Gems level 6 treated as level 5
    return 0;
}

static int is_resource_kind(uint8_t kind) {
    return kind >= 1 && kind <= 6;
}

static const char *resource_kind_name(uint8_t kind);

static uint32_t distance_sq_from_player(const Connection *c, const GatherTile *t) {
    map_pos_t a = getTileMapPosbyPointCode(c->player.zone_id, c->player.point_id);
    map_pos_t b = getTileMapPosbyPointCode(t->zone_id, t->point_id);
    int64_t dx = (int64_t)a.x - (int64_t)b.x;
    int64_t dy = (int64_t)a.y - (int64_t)b.y;
    if (dx < 0) dx = -dx;
    if (dy < 0) dy = -dy;
    if (dx > 32767 || dy > 32767) return UINT32_MAX;
    return (uint32_t)(dx * dx + dy * dy);
}

static int find_tile(const Connection *c, GatherTile *out) {
    const GatheringState *g = &c->gathering;
    int found = 0;
    uint32_t best_dist = UINT32_MAX;
    uint32_t best_count = 0;
    uint8_t best_level = 0;
    uint16_t inactive = 0, unverified = 0, bad_kind = 0, resource_off = 0, level_off = 0;
    uint16_t low_count = 0, bad_distance = 0, not_clearable = 0, bad_schedule = 0, recent_tile = 0;
    uint16_t near_city = 0;

    for (uint16_t i = 0; i < g->tile_count; ++i) {
        const GatherTile *t = &g->tiles[i];
        if (!t->active) { inactive++; continue; }
        /* Never march a tile whose resource classification was not verified.
           This is the hard gate that prevents monster/NPC points (pointKind
           misread by the old scanner) from ever becoming gather targets. */
        if (!t->verified) { unverified++; continue; }
        /* Do not immediately redispatch the same resource point. The server
           can take several seconds to publish the new march state, and a
           second 6615 for the same tile can otherwise create duplicate
           submissions when the scan timer fires again. */
        if (g->recent_tile_until != 0 && now_sec() < g->recent_tile_until &&
            t->zone_id == g->recent_tile_zone_id && t->point_id == g->recent_tile_point_id) {
            recent_tile++; continue;
        }
        if (!is_resource_kind(t->point_kind)) { bad_kind++; continue; }
        if (!resource_enabled(g, t->point_kind)) { resource_off++; continue; }
        if (!level_enabled(g, t->level)) { level_off++; continue; }
        if (t->count < g->minimum_tile_count) { low_count++; continue; }

        uint32_t d = distance_sq_from_player(c, t);
        if (d == UINT32_MAX) { bad_distance++; continue; }
        /* Near-city ring and south-floor gates apply ONLY to the sparse
           experimental path, where the server resolves the destination from a
           pre-wired open-session and a near/non-south node cannot be classified.
           The reference bot's default full/141 MarchEventData path (self-
           contained zone/point/kind/level) has NO such gates and dispatches to
           any verified real resource node, so skip them on that path. */
        if (c->gathering.send_6615_sparse) {
        /* A destination on the city's own protected near tiles is answered by
           the server with EMET_Standby, never dispatched as a gather march.
           Only dispatch toward a real node outside that ring. */
        if (g->min_gather_distance > 0 &&
            d < (uint32_t)g->min_gather_distance * (uint32_t)g->min_gather_distance) {
            near_city++; continue;
        }
        /* South floor: the euclidean gate above only bounds |d|, so an
           at-latitude EAST node (dist ~= dx) can still sit on the city's
           protected latitude and be answered Standby.  The proven type-7
           gather was ~57 units south.  Require the destination to clear the
           city's latitude by at least min_south_units so it matches that
           ring-free geometry. */
        if (g->min_south_units > 0) {
            map_pos_t a = getTileMapPosbyPointCode(c->player.zone_id, c->player.point_id);
            map_pos_t b = getTileMapPosbyPointCode(t->zone_id, t->point_id);
            if ((int32_t)b.y - (int32_t)a.y < (int32_t)g->min_south_units) {
                near_city++; continue;
            }
        }
        /* end sparse-only near-city gates */
        }
        if (g->max_travel_seconds > 0) {
            uint32_t est_travel = (uint32_t)(sqrt((double)d) * 1.5);
            if (est_travel > g->max_travel_seconds) { bad_distance++; continue; }
        }

        if (g->clearable_only) {
            uint32_t required = 10000;
            if (t->level >= 5) required = 300000;
            else if (t->level == 4) required = 150000;
            else if (t->level == 3) required = 50000;
            if (c->troop.total < required) { not_clearable++; continue; }
        }

        if (!IsInSchedule(g, now_sec())) { bad_schedule++; continue; }

        if (!found) {
            *out = *t; found = 1; best_dist = d; best_count = t->count; best_level = t->level; continue;
        }

        int better = 0;
        if (g->highest_level_first) {
            if (t->level > best_level) better = 1;
            else if (t->level == best_level && d < best_dist) better = 1;
        } else if (g->lowest_resource) {
            if (t->count < best_count) better = 1;
            else if (t->count == best_count && d < best_dist) better = 1;
        } else if (d < best_dist) {
            better = 1;
        }
        if (better) {
            *out = *t; best_dist = d; best_count = t->count; best_level = t->level;
        }
    }

    if (!found && g->tile_count > 0) {
        LOGI("[GATHER DEBUG] tile rejection: total=%u inactive=%u unverified=%u bad_kind=%u resource_off=%u level_off=%u low_count=%u bad_distance=%u near_city=%u not_clearable=%u schedule=%u",
             g->tile_count, inactive, unverified, bad_kind, resource_off, level_off, low_count,
             bad_distance, near_city, not_clearable, bad_schedule);
        if (recent_tile) {
            LOGI("[GATHER DEBUG] recently dispatched tile(s) skipped=%u", recent_tile);
        }
    }
    return found;
}

/*
 * Build the troop vector without inventing a march-capacity value that is not
 * present in the current client-side state. When low_tier_first is enabled,
 * use the lowest tier that has available troops and leave higher tiers alone.
 * This preserves higher-tier troops for other activity while keeping the
 * gathering policy deterministic.
 *
 * The 16-slot wire order is verified against SetTroopData_T1_T4 in the
 * supplied v2.200.311 libil2cpp.so.
 */
int build_troop_array(const Connection *c, uint32_t out[16]) {
    memset(out, 0, sizeof(uint32_t) * 16);
    if (!c->troop.loaded) return 0;

    int first_tier = 0;
    int last_tier = 3;

    if (c->gathering.low_tier_first) {
        first_tier = -1;
        for (int tier = 0; tier < 4; ++tier) {
            uint64_t tier_total = (uint64_t)c->troop.siege[tier] +
                                   c->troop.cavalry[tier] +
                                   c->troop.ranged[tier] +
                                   c->troop.infantry[tier];
            if (tier_total > 0) {
                first_tier = tier;
                break;
            }
        }
        if (first_tier < 0) return 0;
        last_tier = first_tier;
    }

    /*
     * MarchEventDataType.SetTroopData_T1_T4 (RVA 0x32927EC, verified in the
     * supplied libil2cpp.so) lays the 16-element wire array out TYPE-MAJOR
     * with the type indices Infantry=0, Ranged=1, Cavalry=2, Siege=3:
     *
     *   slots 0..3   = Infantry T1,T2,T3,T4
     *   slots 4..7   = Ranged   T1,T2,T3,T4
     *   slots 8..11  = Cavalry  T1,T2,T3,T4
     *   slots 12..15 = Siege    T1,T2,T3,T4
     */
    const uint32_t *types[4] = {
        c->troop.infantry, c->troop.ranged, c->troop.cavalry, c->troop.siege
    };
    for (int type = 0; type < 4; ++type) {
        for (int tier = first_tier; tier <= last_tier; ++tier) {
            out[type * 4 + tier] = types[type][tier];
        }
    }

    if (c->gathering.spare_army) {
        for (int i = 0; i < 16; ++i) {
            if (out[i] > 0) { out[i]--; break; }
        }
    }

    uint64_t total = 0;
    for (int i = 0; i < 16; ++i) total += out[i];
    return total != 0;
}

void GatheringInit(Connection *c) {
    GatheringState *g = &c->gathering;
    memset(g, 0, sizeof(*g));
    g->gbg_enabled = false;
    g->chaos_enabled = false;
    g->pending_venue = GATHER_VENUE_KINGDOM;
    for (int v = 0; v < GATHER_VENUE_COUNT; ++v) {
        g->venue_entered[v] = false;
        g->venue_last_req[v] = 0;
        g->venue_suspend_until[v] = 0;
    }
    g->spare_army = true;
    g->highest_level_first = true;
    g->low_tier_first = true;
    g->lowest_resource = false;
    g->clearable_only = false;
    g->gems_ignore_level = true;
    g->recall_camps = false;
    g->gathering_gear = false;
    g->schedule_enabled = false;
    g->schedule_start_min = 0;
    g->schedule_end_min = 1440;
    for (int i = 1; i <= 6; ++i) g->resource_enabled[i] = true;
    for (int i = 1; i <= 5; ++i) g->level_enabled[i] = true;
    g->max_armies = c->player.max_marches ? c->player.max_marches : 1;
    g->delay_seconds = 5;
    g->max_travel_seconds = 900;
    g->search_multiplier = 1;
    g->min_gather_distance = 40;
    g->min_south_units = 50;
    g->scan_reach = 4;
    g->minimum_tile_count = 1;
    g->scan_cursor = 0;
    g->last_scan_base_zone = 0;
}

static bool IsInSchedule(const GatheringState *g, uint32_t now) {
    if (!g->schedule_enabled) return true;
    uint32_t current_min = (now % 86400) / 60;
    uint32_t start = g->schedule_start_min;
    uint32_t end = g->schedule_end_min;
    if (start <= end) {
        return current_min >= start && current_min < end;
    } else {
        // Schedule wraps midnight
        return current_min >= start || current_min < end;
    }
}

static void GatheringVenueTick(Connection *c, GatherVenue venue);
void GatheringTick(Connection *c) {
    /* Kingdom keeps the legacy `.enabled` master switch; GBG and Chaos are
       independent toggles.  Each venue runs its own map-scan but shares the
       single march slot, so at most one venue dispatches at a time. */
    if (c->gathering.enabled)      GatheringVenueTick(c, GATHER_VENUE_KINGDOM);
    if (c->gathering.gbg_enabled)  GatheringVenueTick(c, GATHER_VENUE_GBG);
    if (c->gathering.chaos_enabled) GatheringVenueTick(c, GATHER_VENUE_CHAOS);
}

static void GatheringVenueTick(Connection *c, GatherVenue venue) {
    GatheringState *g = &c->gathering;
    uint32_t now = now_sec();
    /* Per-venue backoff, so the static-stub rejection of one venue (kingdom)
       does not silence gathering in the other venues. */
    if (g->venue_suspend_until[venue] != 0) {
        if (now < g->venue_suspend_until[venue]) return;
        g->venue_suspend_until[venue] = 0;
    }
    /* Kingdom also honours the legacy single `suspend_until`. */
    if (venue == GATHER_VENUE_KINGDOM && g->suspend_until != 0) {
        if (now < g->suspend_until) return;
        g->suspend_until = 0;
    }
    if (!IsInSchedule(g, now)) return;

    if (venue == GATHER_VENUE_KINGDOM) {
        /* GatheringInit runs before role-info, so zone_id is temporarily 0.
           Do not request map data until the real player PointCode is loaded. */
        if (c->player.zone_id == 0) {
            if (g->last_scan_base_zone != 0xFFFFu) {
                LOGI("[GATHER] Waiting for player location before map request");
                g->last_scan_base_zone = 0xFFFFu;
            }
            return;
        }
        if (g->last_scan_base_zone == 0xFFFFu) g->last_scan_base_zone = 0;

        if (g->last_map_request == 0 || now - g->last_map_request >= 5) {
            /* A PointCode zone is a 16x16 zone-grid coordinate: low nibble is
             * the X-zone and the remaining bits are the Y-zone. Sweep outward
             * in both axes over successive scans so a single 2201 request
             * covers a block of the local search area instead of repeatedly
             * asking for only the castle's own zone. */
            uint16_t base = c->player.zone_id;
            int xb = base & 15;
            int yb = base >> 4;

            /* The map request carries four zone IDs.  The PC/mobile client uses
             * the four corners of a 2x2 zone block, not the same zone repeated
             * four times.  Repeating z0 was one of the reasons the gathering
             * cache stayed empty even though the request itself succeeded.
             *
             * A real 6615 gather becomes a genuine GatherMarching (type 7)
             * only toward a real node OUTSIDE the city's own protected ring;
             * near-castle tiles come back Standby.  The old walk never stepped
             * more than one zone off the city's zone, so it only ever cached
             * the near, rejected region.  Sweep the 2x2 block outward in Y
             * (and X) across scan_cursor cycles, up to `scan_reach` zones, so
             * far real nodes enter the cache and find_tile can dispatch to one. */
            uint16_t zones[4];
            int step = g->search_multiplier ? (int)g->search_multiplier : 1;
            int reach = g->scan_reach ? (int)g->scan_reach : 4;
            unsigned cyc = g->scan_cursor++;
            int dX = (cyc & 1) ? reach : 0;        /* 0 or reach zones east  */
            int dY = ((cyc & 6) >> 1) * reach;     /* 0,reach,2reach,3reach south */
            int nx = xb + dX * step;
            int ny = yb + dY * step;
            if (nx > 13) nx = 13;
            if (ny > 61) ny = 61;
            zones[0] = (uint16_t)(nx + (ny << 4));
            zones[1] = (uint16_t)((nx + 1) + (ny << 4));
            zones[2] = (uint16_t)(nx + ((ny + 1) << 4));
            zones[3] = (uint16_t)((nx + 1) + ((ny + 1) << 4));
            RequestMapData(c, 4, zones, true);
            g->last_map_request = now;
            LOGI("[GATHER] Map request sent renew=1 zones=%u,%u,%u,%u point=%u",
                 zones[0], zones[1], zones[2], zones[3], c->player.point_id);
        }
    } else {
        /* Event venues: complete the enter/battlefield-info handshake, then
           keep refreshing it on a throttle.  The incoming RESP (11404 event
           detail / 11993 solo info; and instance-map 2234 routed in main.c) is
           what fills the shared tile cache. */
        if (venue == GATHER_VENUE_GBG && !g->venue_entered[venue]) {
            RequestGuildBattlefieldEventDetail(c);
            RequestGuildBattlefieldEnter(c);
            g->venue_entered[venue] = true;
            g->venue_last_req[venue] = now;
            LOGI("[GATHER GBG] enter handshake sent");
            return;   /* wait for the detail to arrive next tick */
        }
        if (venue == GATHER_VENUE_CHAOS && !g->venue_entered[venue]) {
            RequestSoloBattlefieldInfo(c);
            g->venue_entered[venue] = true;
            g->venue_last_req[venue] = now;
            LOGI("[GATHER CHAOS] info request sent");
            return;   /* wait for RESP_INFO to arrive next tick */
        }
        /* Refresh the event detail/info on a throttle so node state is fresh. */
        if (g->venue_last_req[venue] == 0 || now - g->venue_last_req[venue] >= 60) {
            if (venue == GATHER_VENUE_GBG) RequestGuildBattlefieldEventDetail(c);
            else                          RequestSoloBattlefieldInfo(c);
            g->venue_last_req[venue] = now;
        }
    }
    if (!c->troop.loaded || c->player.max_marches == 0) return;
    uint8_t march_limit = g->max_armies ? g->max_armies : c->player.max_marches;
    if (march_limit > c->player.max_marches && c->player.max_marches != 0)
        march_limit = c->player.max_marches;

    /* The server can lag behind a just-sent 6615.  Count local outstanding
       submissions as reserved march slots so a stale current_marches=0
       cannot cause duplicate/over-capacity submissions. */
    uint16_t effective_marches = c->player.current_marches;
    if (g->local_outstanding_marches > effective_marches)
        effective_marches = g->local_outstanding_marches;
    if (effective_marches >= march_limit) return;

    /* Do not fire duplicate marches while the server is still processing the
       previous gather request.  Some revisions do not update current_marches
       until the response arrives, so relying on that field alone caused the
       same tile to be submitted repeatedly. */
    if (g->pending_until != 0) {
        if (c->player.current_marches > g->pending_base_marches) {
            /* Authoritative march state arrived. */
            g->pending_until = 0;
        } else if (now >= g->pending_until) {
            /*
             * The server did not publish a march by the end of the
             * acknowledgement window.  The authoritative MARCHEVENTDATA
             * snapshot (current_marches) is the ground truth here: it has
             * stayed 0, so no gather march was dispatched.  Release this
             * local reservation so one rejected/stalled request cannot
             * consume a march slot forever, and count the rejection.
             * Repeated Standby-only acks mean the 6615 framing itself is
             * being mis-parsed by the server, so back off instead of
             * re-firing at the next tile in a tight loop.
             */
            if (g->local_outstanding_marches > 0)
                g->local_outstanding_marches--;
            g->pending_until = 0;
            g->standby_rejects++;
            LOGI("[GATHER] ACK window expired without march-state increase; "
                 "released one local reservation (standby_rejects=%u)",
                 (unsigned)g->standby_rejects);
            if (g->standby_rejects >= GATHER_STANDBY_REJECT_LIMIT) {
                g->standby_rejects = 0;
                g->venue_suspend_until[venue] = now + GATHER_STANDBY_SUSPEND_SECONDS;
                if (venue == GATHER_VENUE_KINGDOM)
                    g->suspend_until = now + GATHER_STANDBY_SUSPEND_SECONDS;
                LOGE("[GATHER] %u consecutive 6615 requests were acknowledged "
                     "but never dispatched as a gather march (server kept them "
                     "in Standby, current_marches stayed 0).  The 6615 wire "
                     "format is almost certainly being mis-parsed by the server. "
                     "Suspending gathering for %u seconds; capture a real "
                     "client 6615+2416 exchange to fix the format.",
                     GATHER_STANDBY_REJECT_LIMIT, GATHER_STANDBY_SUSPEND_SECONDS);
            }
        } else {
            return;
        }
    }
    if (g->last_send && now - g->last_send < g->delay_seconds) return;
    GatherTile target;
    /* Never manufacture a destination.  The server validates that the
     * PointCode/PointKind/level describes a real resource tile. */
    if (!find_tile(c, &target)) {
        LOGI("[GATHER] No valid resource tile in cache; waiting for MAPINFO_PLUS");
        return;
    }
    /* Master prompt section 39 requires a RESOURCE TARGET block before any
       gather march is emitted (Kingdom/Zone/Point/X/Y/Kind/Level/Remaining).
       Independently verify the point is a resource point. */
    if (!target.verified) {
        LOGI("[GATHER] refusing unverified target zone=%u point=%u kind=%u -- not a verified resource point",
             target.zone_id, target.point_id, target.point_kind);
        return;
    }
    {
        map_pos_t mp = getTileMapPosbyPointCode(target.zone_id, target.point_id);
        LOGI("RESOURCE TARGET Kingdom=%u Zone=%u Point=%u X:%d Y:%d Kind=%u(%s) Level=%u Remaining=%u Rate=%.2f Time=%llu",
             target.kingdom_id, target.zone_id, target.point_id,
             mp.x, mp.y,
             target.point_kind, resource_kind_name(target.point_kind),
             target.level, target.count, target.rate,
             (unsigned long long)target.time);
    }
    uint32_t troops[16];
    if (!build_troop_array(c, troops)) {
        LOGI("[GATHER] No troops available for gathering");
        return;
    }
    /* Size the march to a real gather body (task #8).  The previous code sent
       EVERY troop of the chosen tier -- on this account 182,347 SIEGE T1 plus
       the full T1 army = 213,192 troops onto a level-4 tile holding 1,575,000
       food.  A real gather march is a small, carry-sized force; shoving 213k
       troops (with siege) onto a 1.5M tile is nonsense the server has no way to
       dispatch, and is the one concrete bot-side anomaly left after ruling out
       the wire format and tile reachability.  A real client sends only enough
       troops to carry the remaining resource and never sends siege to gather,
       so apply both corrections here. */
    {
        /* Siege never gathers: zero the Siege T1..T4 slots (12..15). */
        for (int si = 12; si < 16; ++si) troops[si] = 0;
        /* Cap the total body: tile-count-scaled but bounded. */
        uint32_t cap = target.count / 3u + 1u;   /* ~one troop per 3 food */
        if (cap > GATHER_MARCH_TROOP_CAP) cap = GATHER_MARCH_TROOP_CAP;
        if (cap < GATHER_MARCH_TROOP_MIN) cap = GATHER_MARCH_TROOP_MIN;
        uint64_t have = 0;
        for (int ti = 0; ti < 16; ++ti) have += troops[ti];
        if (have > cap) {
            /* Trim the surplus by cutting largest slots first so a usable
               mixed force survives instead of a quarter-million-troop stack. */
            uint64_t rem = have;
            for (int ti = 15; ti >= 0 && rem > cap; --ti) {
                uint64_t over = rem - cap;
                uint64_t cut  = troops[ti] < over ? troops[ti] : over;
                troops[ti]  -= (uint32_t)cut;
                rem         -= cut;
            }
        }
    }
    /* Hero selection for the march.  Honor the configured gather hero slots;
       if none are set, fall back to the first hero the account actually owns
       (RecvHeroInfo fills Connection.hero).  A gather 6615 march always carries
       at least one real hero id in its body, so we never fabricate a hero:
       with no configured hero and an unloaded or empty roster we do not
       dispatch, and retry next tick after requesting the roster. */
    uint16_t hero_ids[GATHER_HERO_SLOTS];
    for (int hi = 0; hi < GATHER_HERO_SLOTS; ++hi)
        hero_ids[hi] = g->hero_ids[hi];
    {
        bool any_hero = false;
        for (int hi = 0; hi < GATHER_HERO_SLOTS; ++hi)
            if (hero_ids[hi] != 0) { any_hero = true; break; }
        if (!any_hero) {
            if (!c->hero.loaded) {
                RequestHeroInfo(c);
                LOGI("[GATHER] no gather hero configured and hero roster not loaded; "
                     "requested HeroInfo, will retry next tick");
                return;
            }
            if (c->hero.heroes_count == 0) {
                LOGI("[GATHER] no configured hero and account owns no hero; "
                     "cannot send a valid gather march");
                return;
            }
            hero_ids[0] = c->hero.heroes[0].hero_id;
            LOGI("[GATHER] auto-selected first unlocked hero %u for gather march",
                 (unsigned)hero_ids[0]);
        }
    }
    LOGI("[GATHER] gather hero slots: %u,%u,%u,%u,%u",
         (unsigned)hero_ids[0], (unsigned)hero_ids[1],
         (unsigned)hero_ids[2], (unsigned)hero_ids[3],
         (unsigned)hero_ids[4]);
    uint64_t troop_total = 0;
    for (int ti = 0; ti < 16; ++ti) troop_total += troops[ti];
    g->pending_venue = venue;
    /* GATHERING IS ALWAYS A NO-ATTACK MARCH.  The authoritative capture
       (captured_6615_reference.py) proves the real client sends a gather as
       6615 _MSG_REQUEST_TROOPMARCH_NOTATK across venues.  The earlier kingdom
       attempt used 6615 with the OLD fat 175-byte body, which the server read as
       subcommand-4 (standby); with the body now matching the captured sparse
       111-byte layout, we route every venue through the captured 6615 format. */
    bool no_attack = !g->send_2415;
    /* The real gather capture shows the client opening the march UI (0x0478,
       1144, window=7) TWICE, ~1 s apart, immediately before the 6615 dispatch:
         t+0.0  0x0478 window=7     (1st open)
         t+1.0  0x0478 window=7     (2nd open)
         t+2.7  0x19d7 len=111      -> server answers subcommand=0, type=7 (create)
       The lean 6615 body carries only the destination Y (body[78:80]); there is
       NO X, so the server resolves the target from the state armed by the open
       session.  A single open left the server with no create session and it
       answered our far-tile 6615 with subcommand=2 (update).  Mirror the two
       opens so the create session is armed before the 6615.
       When sending via 2415 (full MarchEventData, self-contained coordinates),
       skip both opens so the server keys on the request's own zone/point/type. */
    if (g->send_2415) {
        LOGI("[GATHER] EXPERIMENT: sending via 2415 full MarchEventData (with opens)");
    }
    /* Reveived: the 2415 path previously SKIPPED the two opens and returned
       subcommand=4 (standby, no create).  The sparse 6615 sends these 2 opens
       and DOES create.  Arm the same create-session on the 2415 path too:
       self-contained resource PointKind + an armed create session is the one
       untried shape that could classify a real gather. */
    /* Match the reference bot: the default full/141 MarchEventData path sends
       NO OpenUI before the 6615.  The reference `GatheringTick` dispatches
       RequestTroopMarchGather with no opens and a self-contained 141-byte body
       (zone/point/kind/level in the packet), and that is the working form.
       The 2x OpenUI(1144, window=7) "arm the create-session" behavior applied
       to the sparse experimental path only, so it is gated behind it here;
       firing two back-to-back opens then a 6615 in the same tick was a delta
       from the reference and one of the causes of the server-side standby. */
    if (c->gathering.send_6615_sparse || venue == GATHER_VENUE_KINGDOM) {
        RequestOpenUI(c, 7);
        RequestOpenUI(c, 7);
    }
    bool sent_ok = RequestTroopMarchGather(c, hero_ids, troops,
                                           target.zone_id, target.point_id,
                                           target.point_kind, target.level,
                                           target.count, (uint32_t)troop_total, no_attack,
                                           g->pin_node_id, g->pin_dest_y, venue);
    if (!sent_ok) {
        LOGI("[GATHER] March NOT confirmed at socket layer; state not advanced");
        return;
    }
    g->last_send = now;
    g->pending_until = now + 90;
    if (g->local_outstanding_marches < 255) g->local_outstanding_marches++;
    g->pending_base_marches = c->player.current_marches;
    g->pending_zone_id = target.zone_id;
    g->pending_point_id = target.point_id;
    /* Keep the exact destination on cooldown long enough for the server to
       acknowledge a failed request or publish the resulting march. This does
       not block other resource points. */
    g->recent_tile_zone_id = target.zone_id;
    g->recent_tile_point_id = target.point_id;
    g->recent_tile_until = now + 180;
    LOGI("[GATHER] March socket-send confirmed kind=%u level=%u count=%u zone=%u point=%u troops_total=%u",
           target.point_kind, target.level, target.count, target.zone_id, target.point_id,
           troops[0]+troops[1]+troops[2]+troops[3]+troops[4]+troops[5]+troops[6]+troops[7]+
           troops[8]+troops[9]+troops[10]+troops[11]+troops[12]+troops[13]+troops[14]+troops[15]);
    {
        char ad[120];
        snprintf(ad, sizeof ad, "Gathering %s (Lvl %u / Amount: %u)",
                 resource_kind_name(target.point_kind), (unsigned)target.level,
                 (unsigned)target.count);
        ActivityLogPush(c, ACT_GATHER, ad);
    }
}

static void upsert_tile(Connection *c, uint16_t zone, uint8_t point, uint8_t kind,
                        uint8_t level, uint32_t count, float rate, uint64_t t,
                        uint16_t kingdom, int verified) {
    GatheringState *g = &c->gathering;
    for (uint16_t i = 0; i < g->tile_count; ++i) {
        GatherTile *x = &g->tiles[i];
        if (x->zone_id == zone && x->point_id == point) {
            x->active = true; x->verified = verified ? true : false;
            x->point_kind = kind; x->level = level; x->count = count;
            x->rate = rate; x->time = t; x->kingdom_id = kingdom; return;
        }
    }
    if (g->tile_count >= GATHER_MAX_TILES) return;
    GatherTile *x = &g->tiles[g->tile_count++];
    memset(x, 0, sizeof(*x));
    x->active = true; x->verified = verified ? true : false;
    x->zone_id = zone; x->point_id = point; x->point_kind = kind;
    x->level = level; x->count = count; x->rate = rate; x->time = t; x->kingdom_id = kingdom;
}

static const char *resource_kind_name(uint8_t kind)
{
    switch (kind) {
        case 1: return "FOOD";
        case 2: return "STONE";
        case 3: return "ORE";
        case 4: return "WOOD";
        case 5: return "GOLD";
        case 6: return "GEMS";
        default: return "UNKNOWN";
    }
}

/*
 * v2.201.313 MAPINFO_PLUS (2220) resource record scanner.
 *
 * Real framing derived empirically from 24 live 2220 captures (315 resource
 * records) and cross-confirmed against libil2cpp.so (RecvMapInfoPlus /
 * FixedPipelineCheck).  The old model of a 35-byte ResourcesPoint after every
 * zone/point is NOT what this revision sends:
 *
 *   byte[0]        message variant (0x16, 0x17, 0x0f seen)
 *   bytes[1..2]    u16 flag; (val >> 12) & 0xF = number of zone headers that
 *                  follow, each [zone:u16][updateNum:u64] = 10 bytes
 *   if zone headers > 0: two constant bytes (0x33 0x3c) follow the block
 *   records        fixed 51 bytes each:
 *                    [zone:u16][point:u8][kind:u8][48-byte payload]
 *                  base = 3 + zone_headers*10 + (zone_headers ? 2 : 0)
 *                  (0x2D for a 4-zone packet, 0x03 for a zone-less packet)
 *
 * For resource kinds 1..6 the only meaningful payload fields are:
 *   level = byte[+22]   (observed exclusively 2..5)
 *   count = u32 LE[+23] (observed up to 1,575,000)
 * There is NO time/rate/kingdom field in this framing -- those stay 0 and are
 * reported as such rather than fabricated.  kind 8 (PK_CITY), 9 (PK_CAMP) and
 * 10 (PK_NPC / monster) are non-resources and are skipped, never accepted.
 *
 * The 51-byte stride is trusted only while every step yields a valid zone
 * (<=1023) AND a known point kind.  Mixed packets carry a different-size city
 * tail that desyncs the stride; scanning STOPS on the first invalid step, so a
 * desynced position can never be misread as a resource.  The monster bug tile
 * (zone=506 point=90 kind=10) is rejected by the kind gate before any payload
 * byte is consulted, so it can never become a verified gather target.
 */
static uint32_t parse_native_2220_resources(Connection *c,
                                            const uint8_t *data,
                                            uint16_t size)
{
    uint32_t parsed = 0;
    uint32_t candidates = 0;
    uint16_t reject_badkind = 0, reject_short = 0, reject_badfields = 0;

    if (!data || size < 3 + 51) return 0;

    /* Record base from the leading u16 flag: high nibble = zone header count. */
    uint16_t flag = read_u16(data + 1);
    unsigned zone_headers = (flag >> 12) & 0xFu;
    size_t base = 3 + (size_t)zone_headers * 10;
    if (zone_headers > 0) base += 2;   /* constant 0x33 0x3c marker */

    if (base + 51 > size) {
        ++reject_short;
        LOGI("[GATHER MAP] 2220 base=%zu out of range (size=%u)", base, (unsigned)size);
        return 0;
    }

    for (size_t off = base; off + 51 <= size; off += 51) {
        const uint8_t *rec = data + off;
        uint16_t zone = read_u16(rec + 0);
        uint8_t point = rec[2];
        uint8_t kind = rec[3];

        /* Grid sanity.  Break ONLY on what must be a stride desync: an
           out-of-range zone means the fixed 51-byte stride stopped landing
           on real records.  A valid-but-non-resource kind (yolk, moonstone,
           item-mine, city, camp, NPC/monster) is SKIPPED so a record that
           legitimately carries kind 11..16 does not abort the scan of the
           remaining real resources in the same packet. */
        if (zone > 1023) break;
        if (kind < 1 || kind > 10) { ++reject_badkind; continue; }

        /* Authoritative pointKind: only 1..6 are resource kinds.
           8=PK_CITY, 9=PK_CAMP, 10=PK_NPC (monster) are skipped. */
        if (kind < 1 || kind > 6) { ++reject_badkind; continue; }

        uint8_t level = rec[22];
        uint32_t count = read_u32(rec + 23);

        /* Field gates on top of the kind gate: monster/city records that
           desynced onto a valid kind byte still cannot become resources. */
        if (level < 1 || level > 6)               { ++reject_badfields; continue; }
        if (count == 0 || count > 50000000u)      { ++reject_badfields; continue; }

        ++candidates;

        map_pos_t xy = getTileMapPosbyPointCode(zone, point);
        LOGI("[GATHER TILE CANDIDATE] zone=%u point=%u X:%d Y:%d kind=%u(%s) level=%u count=%u",
             zone, point, xy.x, xy.y, kind, resource_kind_name(kind),
             level, count);

        /* This framing carries no rate/time/kingdom; pass zeros (verified=1). */
        upsert_tile(c, zone, point, kind, level, count, 0.0f, 0ULL, 0, 1);
        ++parsed;
    }

    if (candidates || parsed || reject_badkind || reject_badfields || reject_short) {
        LOGI("[GATHER MAP] resource records=%u accepted=%u rejected: badkind=%u badfields=%u short=%u tiles=%u",
             candidates, parsed, reject_badkind, reject_badfields,
             reject_short, c->gathering.tile_count);
    }
    return parsed;
}


/*
 * Diagnostic capture for MAPINFO_PLUS (2220).
 *
 * This deliberately does NOT interpret the packet.  It records the exact
 * payload delivered by the dispatcher so field boundaries can be derived
 * from real packets rather than from a guessed framing.
 */
#ifdef GATHER_DEBUG_DUMP
/*
 * Append a full hex + ASCII rendering of each 2220 packet to the single
 * shareable file map_2220_dump.txt (capped at CAP_MAX packets so an extended
 * run cannot fill the working directory).  The .bin with the exact bytes is
 * written next to it for offline parsing.  This is ground truth for deriving
 * the real resource-record framing; no interpretation is made here.
 */
static void dump_mapinfo_plus_2220(const uint8_t *data, uint16_t size)
{
    static unsigned seq = 0;
    char bin_name[128];
    FILE *fb = NULL;
    FILE *ft = NULL;

    if (!data || size == 0) return;

    if (seq >= 24) return;
    ++seq;

    snprintf(bin_name, sizeof(bin_name), "map_2220_%04u.bin", seq);
    fb = fopen(bin_name, "wb");
    if (fb) {
        fwrite(data, 1, size, fb);
        fclose(fb);
    }

    ft = fopen("map_2220_dump.txt", "a");
    if (!ft) return;

    fprintf(ft, "=== MAPINFO_PLUS 2220 seq=%u size=%u ===\n", seq, (unsigned)size);
    for (size_t off = 0; off < size; off += 16) {
        size_t n = size - off;
        if (n > 16) n = 16;

        fprintf(ft, "%04zx  ", off);
        for (size_t i = 0; i < 16; ++i) {
            if (i < n) fprintf(ft, "%02X ", data[off + i]);
            else       fprintf(ft, "   ");
        }

        fprintf(ft, " |");
        for (size_t i = 0; i < n; ++i) {
            unsigned char ch = data[off + i];
            fputc((ch >= 32 && ch <= 126) ? ch : '.', ft);
        }
        fprintf(ft, "|\n");
    }
    fclose(ft);

    LOGI("[GATHER MAP] 2220 captured seq=%u size=%u -> %s + map_2220_dump.txt",
         seq, (unsigned)size, bin_name);
}

#endif /* GATHER_DEBUG_DUMP */

void RecvGatherMapInfoPlus(Connection *c, const uint8_t *data, uint16_t size) {
    if (!data || size == 0) return;

#ifdef GATHER_DEBUG_DUMP
    dump_mapinfo_plus_2220(data, size);
#endif

    /* v2.200.31x native resource signatures from real 2220 captures. */
    uint32_t parsed = parse_native_2220_resources(c, data, size);
    if (parsed > 0) {
        LOGI("[GATHER MAP] native resource records=%u tiles=%u",
             parsed, c->gathering.tile_count);
    }
    if (parsed > 0) {
        static uint32_t tile_dump_stamp = 0;
        uint32_t stamp = now_sec();
        if (stamp != tile_dump_stamp) {
            tile_dump_stamp = stamp;
            uint16_t shown = c->gathering.tile_count < 20 ? c->gathering.tile_count : 20;
            for (uint16_t ti = 0; ti < shown; ++ti) {
                const GatherTile *t = &c->gathering.tiles[ti];
                LOGI("[GATHER TILE] #%u active=%u verified=%u zone=%u point=%u kind=%u level=%u count=%u rate=%.2f",
                     ti, t->active ? 1 : 0, t->verified ? 1 : 0, t->zone_id, t->point_id,
                     t->point_kind, t->level, t->count, t->rate);
            }
        }
    }

    /* Do not fall back to the old compact parser: 34/35-byte guesses can
       classify player/monster records as resources on this revision. */
    if (parsed == 0) {
        /* Non-resource 2220 updates are normal (player/city/chat/map state).
           Do not report them as gather errors. */
    }
}

static void GatheringDiagBytes(Connection *c, const char *venue, uint16_t msg,
                               const uint8_t *data, uint16_t size) {
    (void)c;
    if (!data || size == 0) {
        LOGI("[GATHER %s] msg=%u size=0 (no payload)", venue, (unsigned)msg);
        return;
    }
    LOGI("[GATHER %s] msg=%u size=%u raw:", venue, (unsigned)msg, (unsigned)size);
    for (size_t off = 0; off < size; off += 16) {
        char hex[96] = {0};
        char *hp = hex;
        size_t row_end = off + 16;
        if (row_end > size) row_end = size;
        for (size_t i = off; i < row_end; ++i)
            hp += sprintf(hp, "%02X%s", data[i], (i + 1 == row_end) ? "" : " ");
        LOGI("[GATHER %s]   %04zu: %s |%.*s|",
             venue, off, hex, (int)(row_end - off),
             (const char *)(data + off));
    }
}

void RecvGbgEventDetail(Connection *c, const uint8_t *data, uint16_t size) {
    GatheringDiagBytes(c, "GBG", (uint16_t)_MSG_RESP_ALLIANCE_BATTLEFIELD_RUNNING_DETAIL,
                       data, size);
    c->gathering.venue_entered[GATHER_VENUE_GBG] = true;
    uint32_t parsed = parse_native_2220_resources(c, data, size);
    if (parsed > 0)
        LOGI("[GATHER GBG] parsed %u resource records into tile cache (tiles=%u)",
             (unsigned)parsed, (unsigned)c->gathering.tile_count);
}

void RecvChaosInfo(Connection *c, const uint8_t *data, uint16_t size) {
    GatheringDiagBytes(c, "CHAOS", (uint16_t)_MSG_RESP_SOLO_BATTLEFIELD_INFO,
                       data, size);
    c->gathering.venue_entered[GATHER_VENUE_CHAOS] = true;
    uint32_t parsed = parse_native_2220_resources(c, data, size);
    if (parsed > 0)
        LOGI("[GATHER CHAOS] parsed %u resource records into tile cache (tiles=%u)",
             (unsigned)parsed, (unsigned)c->gathering.tile_count);
}

void GatheringOnMarchAck(Connection *c, uint32_t event_type) {
    GatheringState *g = &c->gathering;
    g->last_march_ack_result = event_type;
    g->march_ack_seen = true;

    LOGI("[GATHER ACK] march_event=%u current_marches=%u outstanding=%u pending_until=%u target=%u/%u",
         (unsigned)event_type,
         (unsigned)c->player.current_marches,
         (unsigned)g->local_outstanding_marches,
         (unsigned)g->pending_until,
         (unsigned)g->pending_zone_id,
         (unsigned)g->pending_point_id);

    /*
     * Live behaviour (verified over both the 141-byte and 111-byte 6615
     * senders): the server answers every 6615 with a 2416 whose type byte is
     * 0 (EMET_Standby) and status 0, and the authoritative
     * MARCHEVENTDATA snapshot (RecvMarchData) keeps current_marches at 0.
     * The type byte is what the server classified the march as; a real
     * gather request is classified 7 (GatherMarching) or 2 (Gathering).  A
     * Standby classification with no march-state increase means the server
     * never dispatched a gather march from our request, so counting it as a
     * success was self-deceiving.  Only count type 2/7 as a confirmed gather
     * march.  For any other type, keep the local reservation and the pending
     * window open so an authoritative snapshot can still clear them if the
     * server dispatches late; GatheringTick's expiry branch then counts the
     * rejection and backs off.
     */
    bool is_gather = (event_type == 7 || event_type == 2);
    if (is_gather) {
        if (g->local_outstanding_marches > 0)
            g->local_outstanding_marches--;
        if (c->player.current_marches < c->player.max_marches)
            c->player.current_marches++;
        g->pending_until = 0;
        g->standby_rejects = 0;
        LOGI("[GATHER ACK] gather march confirmed event=%u current_marches=%u",
             (unsigned)event_type,
             (unsigned)c->player.current_marches);
    } else {
        LOGI("[GATHER ACK] ack type=%u is NOT a gather march (expected 2/7); "
             "not counted; awaiting authoritative march snapshot",
             (unsigned)event_type);
    }
}

void GatheringOnMarchReject(Connection *c, uint8_t result_code) {
    GatheringState *g = &c->gathering;
    g->last_march_ack_result = (uint32_t)(0x100u | result_code);
    g->march_ack_seen = true;

    if (g->local_outstanding_marches > 0)
        g->local_outstanding_marches--;
    g->pending_until = 0;

    LOGI("[GATHER ACK] server rejected march request result=%u current_marches=%u target=%u/%u",
         (unsigned)result_code,
         (unsigned)c->player.current_marches,
         (unsigned)g->pending_zone_id,
         (unsigned)g->pending_point_id);
}

void GatheringOnMarchData(Connection *c, uint8_t server_current_marches) {
    GatheringState *g = &c->gathering;
    /*
     * A fresh authoritative march snapshot confirms at least this many
     * marches exist.  If it covers our locally reserved submissions, clear
     * those reservations; otherwise keep the conservative local count.
     */
    if (server_current_marches >= g->local_outstanding_marches)
        g->local_outstanding_marches = 0;
    if (server_current_marches > 0) {
        g->pending_until = 0;
        /* A real march exists: whatever caused the Standby acks is no longer
           relevant, clear the rejection streak. */
        g->standby_rejects = 0;
    }
}

void GatheringOnMarchEnd(Connection *c) {
    GatheringState *g = &c->gathering;
    if (g->local_outstanding_marches > 0)
        g->local_outstanding_marches--;
    if (g->local_outstanding_marches == 0)
        g->pending_until = 0;
}

int GatheringFindTiles(Connection *c, int kind, int level, char *out, size_t out_size)
{
    if (!out || out_size == 0) return 0;
    size_t used = 0; int found = 0;
    for (uint16_t i = 0; i < c->gathering.tile_count && found < 10; ++i) {
        const GatherTile *t = &c->gathering.tiles[i];
        if (!t->active) continue;
        if (!t->verified) continue; /* never present unverified tiles as resources */
        if (kind > 0 && t->point_kind != (uint8_t)kind) continue;
        if (level > 0 && t->level != (uint8_t)level) continue;
        map_pos_t p = getTileMapPosbyPointCode(t->zone_id, t->point_id);
        int n = snprintf(out + used, out_size - used, "%sK:%u X:%u Y:%u L%u C%u",
                         found ? "\n" : "", t->kingdom_id, p.x, p.y, t->level, t->count);
        if (n < 0 || (size_t)n >= out_size - used) break;
        used += (size_t)n; found++;
    }
    return found;
}