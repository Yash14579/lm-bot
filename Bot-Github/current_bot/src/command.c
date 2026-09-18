#include "map_point.h"
#include "command.h"
#include <ctype.h>
#include <stdlib.h>
#include "items.h"
#include "protocol.h"
#include "bank.h"
#include "gathering.h"

/*
 * NOTE:
 *
 * This module is currently a work in progress.
 * The implementation is functional but not considered final.
 * Additional optimizations, structural improvements, and new features
 * are expected as the project continues to evolve.
 */
 
/*void format_number2(uint64_t num, char *out, size_t size) {
    if (num >= 1000000000ULL) {
    	snprintf(out, size, "%.*fB", 2, num / 1000000000.0);
        // snprintf(out, size, "%lluB", num / 1000000000ULL);
    } else if (num >= 1000000ULL) {
    	snprintf(out, size, "%.*fM", 2, num / 1000000.0);
        // snprintf(out, size, "%lluM", num / 1000000ULL);
    } else if (num >= 1000ULL) {
    	snprintf(out, size, "%.*fK", 2, num / 1000.0);
        // snprintf(out, size, "%lluK", num / 1000ULL);
    } else {
        snprintf(out, size, "%lu", num);
    }
}*/


void ShowBankBalance(Connection *c, const char *player_name);
uint64_t GetBagFood(Connection *c);
uint64_t GetBagRock(Connection *c);
uint64_t GetBagWood(Connection *c);
uint64_t GetBagOre(Connection *c);
uint64_t GetBagGold(Connection *c);


void SuperUserAccess(Connection *c,
                     const char *player_name,
                     const char *new_admin)
{
	
	
    // Only current admin may change admin 
    if (strcmp(player_name, c->bot.admin_name) != 0)
    {
        RequestSendMail(
            c,
            player_name,
            "Unauthorized",
            "You don't have permission to grant admin access."
        );
        return;
    }
    if (new_admin == NULL || new_admin[0] == '\0')
    {
        return;
    }

    snprintf(c->bot.admin_name,
             sizeof(c->bot.admin_name),
             "%s",
             new_admin);

    RequestSendMailFmt(
        c,
        player_name,
        "Admin Updated",
        "%s is now the system administrator.",
        c->bot.admin_name
    );
}
static const char *GetCleanPlayerName(const char *player_name)
{
    if (player_name == NULL)
        return "";

    const char *closing_bracket = strchr(player_name, ']');

    if (player_name[0] == '[' && closing_bracket != NULL)
    {
        const char *name = closing_bracket + 1;

        /* Skip spaces after guild tag */
        while (*name == ' ')
            name++;

        return name;
    }

    return player_name;
}
static void RelocateCommand(Connection *c,
                            const char *player_name,
                            const char *message)
{
    unsigned int x = 0;
    unsigned int y = 0;

    /* Admin only */
    if (strcmp(player_name, c->bot.admin_name) != 0)
    {
        RequestSendMail(
            c,
            player_name,
            "Unauthorized",
            "Only the bank administrator can relocate this account."
        );
        return;
    }

    /* Expected: relocate X:106 Y:972 */
    /* Parse coordinates from: relocate X:106 Y:972 */
const char *x_ptr = strchr(message, 'X');
if (x_ptr == NULL)
    x_ptr = strchr(message, 'x');

const char *y_ptr = strchr(message, 'Y');
if (y_ptr == NULL)
    y_ptr = strchr(message, 'y');

int got_x = 0;
int got_y = 0;

if (x_ptr != NULL)
    got_x = sscanf(x_ptr + 1, " : %u", &x);

if (y_ptr != NULL)
    got_y = sscanf(y_ptr + 1, " : %u", &y);

if (got_x != 1 || got_y != 1)
{
    printf(
        "[RELOCATE] Parse failed. Raw='%s' got_x=%d got_y=%d\n",
        message,
        got_x,
        got_y
    );

    RequestSendMail(
        c,
        player_name,
        "Relocate",
        "Usage: $relocate X:106 Y:972"
    );
    return;
}

printf(
    "[RELOCATE] Parsed successfully: X=%u Y=%u\n",
    x,
    y
);

    if (x >= 512 || y >= 1024)
    {
        RequestSendMail(
            c,
            player_name,
            "Relocate",
            "Invalid coordinates."
        );
        return;
    }

    PointCode target = getPointCodeByMapPos(
        (uint16_t)x,
        (uint16_t)y
    );

    printf(
        "[RELOCATE] Admin=%s K=%u X=%u Y=%u Zone=%u Point=%u\n",
        player_name,
        c->player.current_kingdom_id,
        x,
        y,
        target.zoneID,
        target.pointID
    );

    RequestUseAdvancedRelocator(
        c,
        c->player.current_kingdom_id,
        target.zoneID,
        target.pointID
    );
}
uint64_t parse_number_u64(const char *str);


static void SendHelp(Connection *c, const char *player_name)
{
    char prefix = c->bot.command_prefix ? c->bot.command_prefix : '$';
    char buf[1024];
    snprintf(buf, sizeof(buf),
        "%c help\n"
        "%c pos | %c shield | %c shield deploy\n"
        "%c hospital | %c heal | %c heal instant | %c heal finish | %c heal cancel\n"
        "%c troops | %c wounded | %c rallies\n"
        "%c food <amount> | %c stone <amount> | %c wood <amount> | %c ore <amount> | %c gold <amount>\n"
        "%c bal | %c adminbal | %c adminbag | %c banktotal\n"
        "%c setbal <player> <resource> <amount> | %c addbal <player> <resource> <amount>\n"
        "%c chat <msg> | %c gchat <msg> | %c join <name> <amount> [inf range cav siege]\n"
        "%c relocate X:<x> Y:<y>\n"
        "\n"
        "Bank: payransom clearboard ess stats pstats gryphon reguser unreguser pos shield deploy relocatekvk migrate recall buildspam hunt addtitle deltitle whitelist blacklist unlistwhite unlistblack purge abort yell quest guild camp campleader snowbeast stop reloadacc members busrank resetstats joingvg leavegvg joinca leaveca joinda leaveda\n"
        "Search: findtile findmonster findnest\n"
        "Balance: adminbal adminbag setbal setacc transfer setrsslimit\n"
        "Resource: donate admindonate adminrss\n"
        "Supported aliases from the Guild Bank command set are being mapped to verified game protocols.",
        prefix, prefix, prefix, prefix,
        prefix, prefix, prefix, prefix, prefix,
        prefix, prefix, prefix,
        prefix, prefix, prefix, prefix, prefix,
        prefix, prefix, prefix, prefix, prefix, prefix,
        prefix, prefix, prefix, prefix,
        prefix, prefix, prefix,
        prefix
    );
    RequestSendMail(c, player_name, "Commands", buf);
}


static void SendResources(Connection *c, const char *player_name)
{
    char f[20], r[20], w[20], o[20], g[20];
    format_number2(c->resources.food, f, sizeof(f));
    format_number2(c->resources.rock, r, sizeof(r));
    format_number2(c->resources.wood, w, sizeof(w));
    format_number2(c->resources.ore,  o, sizeof(o));
    format_number2(c->resources.gold, g, sizeof(g));

    RequestSendMailFmt(c, player_name, "Resources",
        "RSS: Food %s | Stone %s | Wood %s | Ore %s | Gold %s",
        f, r, w, o, g);
}

static void SendPosition(Connection *c, const char *player_name)
{
    map_pos_t pos = getTileMapPosbyPointCode(c->player.zone_id, c->player.point_id);
    RequestSendMailFmt(c, player_name, "Position",
        "K:%u X:%u Y:%u | Zone:%u Point:%u",
        c->player.current_kingdom_id, pos.x, pos.y,
        c->player.zone_id, c->player.point_id);
}

static void SendTroops(Connection *c, const char *player_name)
{
    if (!c->troop.loaded) {
        RequestSendMail(c, player_name, "Troops", "Troop data is not loaded yet.");
        return;
    }

    RequestSendMailFmt(c, player_name, "Troops",
        "Total: %u\n"
        "Inf T1:%u T2:%u T3:%u T4:%u\n"
        "Rng T1:%u T2:%u T3:%u T4:%u\n"
        "Cav T1:%u T2:%u T3:%u T4:%u\n"
        "Siege T1:%u T2:%u T3:%u T4:%u",
        c->troop.total,
        c->troop.infantry[0], c->troop.infantry[1],
        c->troop.infantry[2], c->troop.infantry[3],
        c->troop.ranged[0], c->troop.ranged[1],
        c->troop.ranged[2], c->troop.ranged[3],
        c->troop.cavalry[0], c->troop.cavalry[1],
        c->troop.cavalry[2], c->troop.cavalry[3],
        c->troop.siege[0], c->troop.siege[1],
        c->troop.siege[2], c->troop.siege[3]);
}

static void SendWounded(Connection *c, const char *player_name)
{
    if (!c->wounded.loaded) {
        RequestSendMail(c, player_name, "Hospital", "Hospital data is not loaded yet.");
        return;
    }

    RequestSendMailFmt(c, player_name, "Hospital",
        "Wounded: %u | Currently healing: %u | Queue: %us",
        c->wounded.troop.total,
        c->wounded.healing.total,
        c->wounded.total_time);
}

static void SendRallyList(Connection *c, const char *player_name)
{
    RequestRallyList(c);
    RequestSendMailFmt(c, player_name, "Rallies",
        "Rally list requested. Active:%u Incoming:%u. "
        "Check bot console for returned rally entries.",
        c->rally_status.active_rally_count,
        c->rally_status.being_rally_count);
}

static int FindRallyLeader(Connection *c, uint32_t index, char out[13])
{
    if (index < 30 && c->npc_rallies[index].ally_name[0] != '\0') {
        snprintf(out, 13, "%s", c->npc_rallies[index].ally_name);
        return 1;
    }
    if (index < 30 && c->ally_rallies[index].ally_name[0] != '\0') {
        snprintf(out, 13, "%s", c->ally_rallies[index].ally_name);
        return 1;
    }
    return 0;
}

static int ParsePercentTroops(uint32_t total, int inf, int ranged, int cav, int siege,
                              uint32_t troop[16])
{
    if (inf < 0 || ranged < 0 || cav < 0 || siege < 0 ||
        inf > 100 || ranged > 100 || cav > 100 || siege > 100)
        return 0;

    if (inf + ranged + cav + siege > 100)
        return 0;

    memset(troop, 0, sizeof(uint32_t) * 16);

    uint32_t kind_total[4] = {
        (uint32_t)((uint64_t)total * (uint32_t)inf / 100),
        (uint32_t)((uint64_t)total * (uint32_t)ranged / 100),
        (uint32_t)((uint64_t)total * (uint32_t)cav / 100),
        (uint32_t)((uint64_t)total * (uint32_t)siege / 100)
    };

    /* Keep the command simple: distribute each kind evenly by tier.
       The packet itself still carries all 16 troop slots. */
    for (int kind = 0; kind < 4; ++kind) {
        uint32_t each = kind_total[kind] / 4;
        uint32_t rem = kind_total[kind] % 4;
        for (int tier = 0; tier < 4; ++tier)
            troop[kind * 4 + tier] = each + (tier < (int)rem ? 1 : 0);
    }
    return 1;
}

static void JoinCommand(Connection *c, const char *player_name, const char *args)
{
    char leader[32] = {0};
    uint64_t amount64 = 0;
    int inf = 100, ranged = 0, cav = 0, siege = 0;

    if (sscanf(args, "%31s %31s %d %d %d %d",
               leader, (char *)&amount64, &inf, &ranged, &cav, &siege) < 2) {
        RequestSendMail(c, player_name, "Join",
            "Usage: $join <leader> <amount> [inf range cav siege]");
        return;
    }

    /* Re-parse amount safely; the second token may have K/M/B suffix. */
    char amount_str[32] = {0};
    if (sscanf(args, "%31s %31s", leader, amount_str) != 2) {
        RequestSendMail(c, player_name, "Join",
            "Usage: $join <leader> <amount> [inf range cav siege]");
        return;
    }

    uint64_t total64 = parse_number_u64(amount_str);
    if (total64 == 0 || total64 > UINT32_MAX) {
        RequestSendMail(c, player_name, "Join", "Invalid troop amount.");
        return;
    }

    /* If the first argument is numeric, resolve it against the cached rally arrays. */
    char resolved[13] = {0};
    char *endp = NULL;
    unsigned long idx = strtoul(leader, &endp, 10);
    if (*leader != '\0' && endp && *endp == '\0') {
        if (!FindRallyLeader(c, (uint32_t)idx, resolved)) {
            RequestSendMail(c, player_name, "Join",
                "Rally index is not cached. Use $rallies first.");
            return;
        }
    } else {
        snprintf(resolved, sizeof(resolved), "%s", leader);
    }

    uint32_t troops[16];
    if (!ParsePercentTroops((uint32_t)total64, inf, ranged, cav, siege, troops)) {
        RequestSendMail(c, player_name, "Join",
            "Percentages must be 0..100 and total <= 100.");
        return;
    }

    RequestJoinRally(c, resolved, troops);

    RequestSendMailFmt(c, player_name, "Join",
        "Join request sent to %s for %u troops.",
        resolved, (uint32_t)total64);
}

static void ChatCommand(Connection *c, const char *player_name,
                        const char *args, uint8_t channel)
{
    while (*args == ' ') args++;
    if (*args == '\0') {
        RequestSendMail(c, player_name, "Chat", "Usage: $chat <message>");
        return;
    }

    RequestSendChat(c, channel, args);
    RequestSendMail(c, player_name, "Chat", "Message sent.");
}

static void StopTransferCommand(Connection *c, const char *player_name)
{
    if (c->transfer.state == TRANSFER_IDLE) {
        RequestSendMail(c, player_name, "Transfer", "No active resource transfer.");
        return;
    }

    memset(&c->transfer, 0, sizeof(c->transfer));
    c->transfer.state = TRANSFER_IDLE;
    RequestSendMail(c, player_name, "Transfer", "Resource transfer cancelled.");
}

static int IsAdmin(Connection *c, const char *name)
{
    return strcmp(c->bot.admin_name, name) == 0;
}

static void AdminLedgerCommand(Connection *c, const char *player_name,
                               const char *message, int add)
{
    if (!IsAdmin(c, player_name)) {
        RequestSendMail(c, player_name, "Unauthorized", "Admin only.");
        return;
    }

    char target[32], type[16], amount_str[32];
    if (sscanf(message, "%31s %31s %31s", target, type, amount_str) != 3) {
        RequestSendMail(c, player_name, "Bank",
            add ? "Usage: $addbal <player> <resource> <amount>"
                : "Usage: $setbal <player> <resource> <amount>");
        return;
    }

    uint64_t amount = parse_number_u64(amount_str);
    if (amount > INT64_MAX) {
        RequestSendMail(c, player_name, "Bank", "Amount is too large.");
        return;
    }

    if (add)
        add_balance(target, type, (long)amount);
    else
        set_balance(target, type, (long)amount);

    char text[256];
    get_balance_text(target, text);
    RequestSendMail(c, player_name, "Bank", text);
}


static int IsAdmin(Connection *c, const char *name);

static void SendShieldStatus(Connection *c, const char *player_name)
{
    if (!c->shield_info.loaded) {
        RequestSendMail(c, player_name, "Shield",
                        "Shield data is not loaded yet.");
        return;
    }

    if (!c->shield_info.active) {
        RequestSendMail(c, player_name, "Shield",
                        "No active shield.");
        return;
    }

    uint64_t end_time = c->shield_info.begin_time +
                        (uint64_t)c->shield_info.duration;
    uint64_t remaining = end_time > c->server_time
                       ? end_time - c->server_time : 0;

    if (remaining == 0) {
        c->shield_info.active = false;
        RequestSendMail(c, player_name, "Shield",
                        "Shield has expired.");
        return;
    }

    RequestSendMailFmt(c, player_name, "Shield",
        "%s | Remaining: %s | Item:%u",
        GetShieldName(c->shield_info.item_id),
        FormatTime((uint32_t)(remaining > UINT32_MAX ? UINT32_MAX : remaining)),
        c->shield_info.item_id);
}

static void DeployShieldCommand(Connection *c, const char *player_name)
{
    if (!IsAdmin(c, player_name)) {
        RequestSendMail(c, player_name, "Unauthorized",
                        "Only the bank administrator can deploy the shield.");
        return;
    }

    if (!c->shield_info.loaded) {
        RequestSendMail(c, player_name, "Shield",
                        "Shield inventory/status is not loaded yet.");
        return;
    }

    if (c->shield_info.active) {
        SendShieldStatus(c, player_name);
        return;
    }

    DeployBestShield(c);
    RequestSendMail(c, player_name, "Shield",
                    "Shield activation request sent. Check !shield for the result.");
}

static void SendHospitalStatus(Connection *c, const char *player_name)
{
    if (!c->wounded.loaded) {
        RequestSendMail(c, player_name, "Hospital",
                        "Hospital data is not loaded yet.");
        return;
    }

    RequestSendMailFmt(c, player_name, "Hospital",
        "Wounded: %u\nHealing now: %u\nQueue time: %s",
        c->wounded.troop.total,
        c->wounded.healing.total,
        FormatTime(c->wounded.total_time));
}

static int BuildAllWoundedPacket(Connection *c, uint32_t out[16])
{
    memset(out, 0, sizeof(uint32_t) * 16);

    if (!c->wounded.loaded)
        return 0;

    /* dump.cs exposes 16 hospital soldier slots:
       infantry/ranged/cavalry/siege × four tiers. */
    for (int i = 0; i < 4; ++i) {
        out[i]      = c->wounded.troop.infantry[i];
        out[4 + i]  = c->wounded.troop.ranged[i];
        out[8 + i]  = c->wounded.troop.cavalry[i];
        out[12 + i] = c->wounded.troop.siege[i];
    }

    return c->wounded.troop.total != 0;
}

static void HealCommand(Connection *c, const char *player_name, const char *args)
{
    if (!c->wounded.loaded) {
        RequestSendMail(c, player_name, "Hospital",
                        "Hospital data is not loaded yet.");
        return;
    }

    while (*args == ' ') args++;

    if (strcmp(args, "finish") == 0) {
        RequestFinishHealing(c);
        RequestSendMail(c, player_name, "Hospital",
                        "Finish-healing request sent.");
        return;
    }

    if (strcmp(args, "cancel") == 0) {
        RequestCancelHealing(c);
        RequestSendMail(c, player_name, "Hospital",
                        "Cancel-healing request sent.");
        return;
    }

    uint32_t troops[16];
    if (!BuildAllWoundedPacket(c, troops)) {
        RequestSendMail(c, player_name, "Hospital",
                        "No wounded T1-T4 troops are available.");
        return;
    }

    if (strcmp(args, "instant") == 0) {
        RequestInstantHealing(c, troops);
        RequestSendMail(c, player_name, "Hospital",
                        "Instant-healing request sent for all loaded wounded T1-T4 troops.");
        return;
    }

    RequestHealingTroops(c, troops);
    RequestSendMail(c, player_name, "Hospital",
                    "Healing request sent for all loaded wounded T1-T4 troops.");
}

static void SendBankBag(Connection *c, const char *player_name)
{
    if (!IsAdmin(c, player_name)) {
        RequestSendMail(c, player_name, "Unauthorized", "Admin only.");
        return;
    }

    char f[20], s[20], w[20], o[20], g[20];
    format_number2(GetBagFood(c),  f, sizeof(f));
    format_number2(GetBagRock(c),  s, sizeof(s));
    format_number2(GetBagWood(c),  w, sizeof(w));
    format_number2(GetBagOre(c),   o, sizeof(o));
    format_number2(GetBagGold(c),  g, sizeof(g));

    RequestSendMailFmt(c, player_name, "Bank Bag",
        "Food: %s | Stone: %s | Wood: %s | Ore: %s | Gold: %s",
        f, s, w, o, g);
}

static const char *ResourceKindName(uint8_t kind)
{
    switch (kind) {
        case 1: return "food";
        case 2: return "stone";
        case 3: return "ore";
        case 4: return "wood";
        case 5: return "gold";
        case 6: return "gems";
        default: return "?";
    }
}

static int GatherResourceKind(const char *s)
{
    if (!s) return 0;
    if (!strcmp(s, "food")) return 1;
    if (!strcmp(s, "stone")) return 2;
    if (!strcmp(s, "ore") || !strcmp(s, "iron")) return 3;
    if (!strcmp(s, "wood")) return 4;
    if (!strcmp(s, "gold")) return 5;
    if (!strcmp(s, "gems") || !strcmp(s, "gem") || !strcmp(s, "crystal")) return 6;
    return 0;
}

static int ParseOnOff(const char *s, bool *v)
{
    if (!strcmp(s, "on")) { *v = true; return 1; }
    if (!strcmp(s, "off")) { *v = false; return 1; }
    return 0;
}

/* Parse "HH:MM" (24h) into minutes-from-midnight; returns -1 on error. */
static int ParseClockMinutes(const char *s)
{
    if (!s) return -1;
    int h = 0, m = 0;
    char extra[4] = {0};
    if (sscanf(s, "%2d:%2d%3s", &h, &m, extra) != 2) return -1;
    if (h < 0 || h > 23 || m < 0 || m > 59) return -1;
    return h * 60 + m;
}

static void GatherSendStatus(Connection *c, const char *player_name)
{
    GatheringState *g = &c->gathering;

    char res_line[128] = {0};
    for (int i = 1; i <= 6; ++i) {
        char item[24];
        snprintf(item, sizeof(item), "%s:%s", ResourceKindName((uint8_t)i),
                 g->resource_enabled[i] ? "on" : "off");
        strncat(res_line, item, sizeof(res_line) - strlen(res_line) - 1);
        if (i < 6) strncat(res_line, " ", sizeof(res_line) - strlen(res_line) - 1);
    }

    char lvl_line[64] = {0};
    for (int i = 1; i <= 5; ++i) {
        char item[16];
        snprintf(item, sizeof(item), "L%d:%s", i, g->level_enabled[i] ? "on" : "off");
        strncat(lvl_line, item, sizeof(lvl_line) - strlen(lvl_line) - 1);
        if (i < 5) strncat(lvl_line, " ", sizeof(lvl_line) - strlen(lvl_line) - 1);
    }

    char hero_line[96] = "none";
    int heroes = 0;
    for (int i = 0; i < GATHER_HERO_SLOTS; ++i) {
        if (g->hero_ids[i] != 0) {
            char item[20];
            snprintf(item, sizeof(item), "%shero%d=%u", heroes ? " " : "", i + 1, g->hero_ids[i]);
            strncat(hero_line, item, sizeof(hero_line) - strlen(hero_line) - 1);
            heroes++;
        }
    }

    RequestSendMailFmt(c, player_name, "Gather",
        "enabled: %s | tiles known: %u | marches: %u/%u\n"
        "\n"
        "resources: %s\n"
        "levels: %s\n"
        "\n"
        "spare-army: %s | highest-level: %s | lowest-resource: %s\n"
        "clearable-only: %s | gems-ignore-level: %s\n"
        "auto-recall-camps: %s | gathering-gear: %s\n"
        "\n"
        "max-armies: %u | delay: %us | travel-max: %us\n"
        "search-multiplier: %u | min-gather-dist: %u | min-south-units: %u | scan-reach: %u | min-tile-count: %u\n"
        "heroes: %s\n"
        "\n"
        "schedule: %s (%02u:%02u-%02u:%02u)",
        g->enabled ? "ON" : "OFF", g->tile_count,
        c->player.current_marches, c->player.max_marches,
        res_line, lvl_line,
        g->spare_army ? "on" : "off",
        g->highest_level_first ? "on" : "off",
        g->lowest_resource ? "on" : "off",
        g->clearable_only ? "on" : "off",
        g->gems_ignore_level ? "on" : "off",
        g->recall_camps ? "on" : "off",
        g->gathering_gear ? "on" : "off",
        g->max_armies, g->delay_seconds, g->max_travel_seconds,
        g->search_multiplier, g->min_gather_distance, g->min_south_units, g->scan_reach,
        g->minimum_tile_count,
        hero_line,
        g->schedule_enabled ? "ON" : "OFF",
        g->schedule_start_min / 60, g->schedule_start_min % 60,
        g->schedule_end_min / 60, g->schedule_end_min % 60);
}

void command_handler(Connection *c, const char *player_name, const char *message)
{
    if (c == NULL || player_name == NULL || message == NULL)
        return;

    if (c->bot.command_prefix == 0)
        return;

    if (message[0] != c->bot.command_prefix)
        return;

    /* Remove guild tag, e.g. "[B=E] yash1459" -> "yash1459" */
    player_name = GetCleanPlayerName(player_name);

    printf("[COMMAND] Sender: '%s'\n", player_name);

    message++; // skip prefix // skip prefix 
	// ===============================
// SAFE BALANCE COMMANDS
// ===============================

// $bal
if (strcmp(message, "bal") == 0)
{
    char text[512] = {0};
    get_balance_text(player_name, text);
    RequestSendMail(c, player_name, "Balance", text[0] ? text : "No bank record.");
    return;
}

// $adminbal
if (strcmp(message, "adminbal") == 0)
{
    ShowBankBalance(c, player_name);
    return;
}
    if (strcmp(message, "help") == 0) { SendHelp(c, player_name); return; }
    if (strcmp(message, "resources") == 0 || strcmp(message, "rss") == 0) {
        SendResources(c, player_name); return;
    }
    if (strcmp(message, "pos") == 0 || strcmp(message, "position") == 0) {
        SendPosition(c, player_name); return;
    }
    if (strcmp(message, "shield") == 0) {
        SendShieldStatus(c, player_name); return;
    }
    if (strcmp(message, "shield deploy") == 0) {
        DeployShieldCommand(c, player_name); return;
    }
    if (strcmp(message, "hospital") == 0 || strcmp(message, "heal status") == 0) {
        SendHospitalStatus(c, player_name); return;
    }
    if (strcmp(message, "heal") == 0 || strncmp(message, "heal ", 5) == 0) {
        HealCommand(c, player_name, message + 4); return;
    }
    if (strcmp(message, "adminbag") == 0) {
        SendBankBag(c, player_name); return;
    }

    if (strcmp(message, "troops") == 0) { SendTroops(c, player_name); return; }
    if (strcmp(message, "wounded") == 0 || strcmp(message, "hospital") == 0) {
        SendWounded(c, player_name); return;
    }
    if (strcmp(message, "rallies") == 0 || strcmp(message, "rally list") == 0) {
        SendRallyList(c, player_name); return;
    }
    if (strcmp(message, "stop") == 0) {
        StopTransferCommand(c, player_name); return;
    }
    if (strcmp(message, "banktotal") == 0) {
        if (!IsAdmin(c, player_name)) {
            RequestSendMail(c, player_name, "Unauthorized", "Admin only.");
        } else {
            char text[512] = {0};
            get_bank_total(text);
            RequestSendMail(c, player_name, "Bank Total", text);
        }
        return;
    }
    if (strncmp(message, "setbal ", 7) == 0) {
        AdminLedgerCommand(c, player_name, message + 7, 0); return;
    }
    if (strncmp(message, "addbal ", 7) == 0) {
        AdminLedgerCommand(c, player_name, message + 7, 1); return;
    }
    if (strncmp(message, "chat ", 5) == 0) {
        ChatCommand(c, player_name, message + 5, COMMAND_CHANNEL_WORLD); return;
    }
    if (strncmp(message, "gchat ", 6) == 0) {
        ChatCommand(c, player_name, message + 6, COMMAND_CHANNEL_GUILD); return;
    }
    if (strncmp(message, "join ", 5) == 0) {
        JoinCommand(c, player_name, message + 5); return;
    }

	/* $relocate X:106 Y:972 */
if (strncmp(message, "relocate ", 9) == 0)
{
    RelocateCommand(c, player_name, message);
    return;
}
	// handle food command 
	if (memcmp(message, "food", 4) == 0 && (message[4] == '\0' || message[4] == ' '))
	{
		// if (c->bank.enabled || strcmp(c->bot.admin_name, player_name) == 0) {
			ResourceCommandHandler(c, player_name, message + 4, RESOURCE_FOOD, "food");
		// }
		return;
	}
	
	// handle stone command
	if (memcmp(message, "stone", 5) == 0 && (message[5] == '\0' || message[5] == ' '))
	{
		ResourceCommandHandler(c, player_name, message + 5, RESOURCE_ROCK, "stone");
		return;
	}
	
	// handle wood command
	if (memcmp(message, "wood", 4) == 0 && (message[4] == '\0' || message[4] == ' '))
	{
		ResourceCommandHandler(c, player_name, message + 4, RESOURCE_WOOD, "wood");
		return;
	}
	
	// handle ore command
	if (memcmp(message, "ore", 3) == 0 && (message[3] == '\0' || message[3] == ' '))
	{
		ResourceCommandHandler(c, player_name, message + 3, RESOURCE_ORE, "ore");
		return;
	}
	
	// handle gold command
	if (memcmp(message, "gold", 4) == 0 && (message[4] == '\0' || message[4] == ' '))
	{
		ResourceCommandHandler(c, player_name, message + 4, RESOURCE_GOLD, "gold");
		return;
	}
	
	
	if (memcmp(message, "bank bal", 8) == 0 && (message[8] == '\0' || message[8] == ' '))
	{
		ShowBankBalance(c, player_name);
		return;
	}
	
	if (memcmp(message, "su", 2) == 0)
	{
		if (message[2] == '\0') {
			RequestSendMailFmt(
				c,
				player_name,
				"Sudo access",
				"Usage: %csu %s",
				c->bot.command_prefix,
				c->transfer.target_name
			);
			return;
		}

		if (message[2] == ' ') {
			SuperUserAccess(c, player_name, message + 3);
			return;
		}

		return;
	}

	/* ---- Bank commands ---- */
	if (strcmp(message, "payransom") == 0) {
		if (!IsAdmin(c, player_name)) {
			RequestSendMail(c, player_name, "Unauthorized", "Admin only.");
		} else {
			RequestSendMail(c, player_name, "Pay Ransom", "Not implemented: requires ransom payment protocol.");
		}
		return;
	}

	if (strcmp(message, "clearboard") == 0) {
		if (!IsAdmin(c, player_name)) {
			RequestSendMail(c, player_name, "Unauthorized", "Admin only.");
		} else {
			reset_bank();
			RequestSendMail(c, player_name, "Clear Board", "Bank ledger cleared.");
		}
		return;
	}

	if (strcmp(message, "ess") == 0) {
		RequestSendMail(c, player_name, "Essence", "Not implemented: essence query requires alliance protocol.");
		return;
	}

	if (strcmp(message, "stats") == 0) {
		if (!IsAdmin(c, player_name)) {
			RequestSendMail(c, player_name, "Unauthorized", "Admin only.");
		} else {
			char text[512];
			get_bank_total(text);
			RequestSendMailFmt(c, player_name, "Bank Stats",
				"%s\nRegistered users: %d", text, get_registered_count());
		}
		return;
	}

	if (strcmp(message, "pstats") == 0) {
		if (!IsAdmin(c, player_name)) {
			RequestSendMail(c, player_name, "Unauthorized", "Admin only.");
		} else {
			RequestSendMail(c, player_name, "Player Stats", "Not implemented: requires player data enumeration.");
		}
		return;
	}

	if (strcmp(message, "gryphon") == 0) {
		RequestSendMail(c, player_name, "Gryphon", "Not implemented: requires gryphon/beast protocol.");
		return;
	}

	if (strncmp(message, "reguser ", 8) == 0) {
		if (!IsAdmin(c, player_name)) {
			RequestSendMail(c, player_name, "Unauthorized", "Admin only.");
		} else {
			char target[64];
			if (sscanf(message + 8, "%63s", target) == 1) {
				get_player(target);
				RequestSendMailFmt(c, player_name, "Register User", "%s registered.", target);
			} else {
				RequestSendMail(c, player_name, "Register User", "Usage: reguser <name>");
			}
		}
		return;
	}

	if (strncmp(message, "unreguser ", 10) == 0) {
		if (!IsAdmin(c, player_name)) {
			RequestSendMail(c, player_name, "Unauthorized", "Admin only.");
		} else {
			char target[64];
			if (sscanf(message + 10, "%63s", target) == 1) {
				if (del_player(target)) {
					RequestSendMailFmt(c, player_name, "Unregister User", "%s removed.", target);
				} else {
					RequestSendMailFmt(c, player_name, "Unregister User", "%s not found.", target);
				}
			} else {
				RequestSendMail(c, player_name, "Unregister User", "Usage: unreguser <name>");
			}
		}
		return;
	}

	if (strcmp(message, "pos") == 0) {
		SendPosition(c, player_name);
		return;
	}

	if (strcmp(message, "shield") == 0) {
		SendShieldStatus(c, player_name);
		return;
	}

	if (strcmp(message, "shield deploy") == 0) {
		DeployShieldCommand(c, player_name);
		return;
	}

	if (strncmp(message, "relocatekvk ", 12) == 0) {
		if (!IsAdmin(c, player_name)) {
			RequestSendMail(c, player_name, "Unauthorized", "Admin only.");
		} else {
			RequestSendMail(c, player_name, "Relocate KVK", "Not implemented: KVK relocate requires special item.");
		}
		return;
	}

	if (strcmp(message, "migrate") == 0) {
		if (!IsAdmin(c, player_name)) {
			RequestSendMail(c, player_name, "Unauthorized", "Admin only.");
		} else {
			RequestSendMail(c, player_name, "Migrate", "Not implemented: requires migration scroll and kingdom data.");
		}
		return;
	}

	if (strcmp(message, "recall") == 0) {
		if (!IsAdmin(c, player_name)) {
			RequestSendMail(c, player_name, "Unauthorized", "Admin only.");
		} else {
			for (int i = 0; i < c->player.max_marches; i++) {
				RequestTroopRecall(c, i);
			}
			RequestSendMail(c, player_name, "Recall", "Recall sent for all marches.");
		}
		return;
	}

	if (strcmp(message, "buildspam") == 0) {
		if (!IsAdmin(c, player_name)) {
			RequestSendMail(c, player_name, "Unauthorized", "Admin only.");
		} else {
			c->help_spam.active = !c->help_spam.active;
			RequestSendMailFmt(c, player_name, "Build Spam", "%s", c->help_spam.active ? "ON" : "OFF");
		}
		return;
	}

	if (strcmp(message, "hunt") == 0) {
		RequestSendMail(c, player_name, "Hunt", "Not implemented: requires monster search protocol.");
		return;
	}

	if (strncmp(message, "addtitle ", 9) == 0) {
		if (!IsAdmin(c, player_name)) {
			RequestSendMail(c, player_name, "Unauthorized", "Admin only.");
		} else {
			RequestSendMail(c, player_name, "Add Title", "Not implemented: requires alliance title protocol.");
		}
		return;
	}

	if (strncmp(message, "deltitle ", 9) == 0) {
		if (!IsAdmin(c, player_name)) {
			RequestSendMail(c, player_name, "Unauthorized", "Admin only.");
		} else {
			RequestSendMail(c, player_name, "Del Title", "Not implemented: requires alliance title protocol.");
		}
		return;
	}

	if (strncmp(message, "whitelist ", 10) == 0) {
		if (!IsAdmin(c, player_name)) {
			RequestSendMail(c, player_name, "Unauthorized", "Admin only.");
		} else {
			char target[64];
			if (sscanf(message + 10, "%63s", target) == 1) {
				PlayerBalance* p = get_player(target);
				if (p) {
					RequestSendMailFmt(c, player_name, "Whitelist", "%s added (bank access granted).", target);
				}
			} else {
				RequestSendMail(c, player_name, "Whitelist", "Usage: whitelist <name>");
			}
		}
		return;
	}

	if (strncmp(message, "blacklist ", 10) == 0) {
		if (!IsAdmin(c, player_name)) {
			RequestSendMail(c, player_name, "Unauthorized", "Admin only.");
		} else {
			char target[64];
			if (sscanf(message + 10, "%63s", target) == 1) {
				del_player(target);
				RequestSendMailFmt(c, player_name, "Blacklist", "%s removed (bank access revoked).", target);
			} else {
				RequestSendMail(c, player_name, "Blacklist", "Usage: blacklist <name>");
			}
		}
		return;
	}

	if (strncmp(message, "unlistwhite ", 12) == 0) {
		if (!IsAdmin(c, player_name)) {
			RequestSendMail(c, player_name, "Unauthorized", "Admin only.");
		} else {
			char target[64];
			if (sscanf(message + 12, "%63s", target) == 1) {
				del_player(target);
				RequestSendMailFmt(c, player_name, "Unlist White", "%s removed from whitelist.", target);
			} else {
				RequestSendMail(c, player_name, "Unlist White", "Usage: unlistwhite <name>");
			}
		}
		return;
	}

	if (strncmp(message, "unlistblack ", 12) == 0) {
		if (!IsAdmin(c, player_name)) {
			RequestSendMail(c, player_name, "Unauthorized", "Admin only.");
		} else {
			RequestSendMail(c, player_name, "Unlist Black", "Not implemented: requires persistent blacklist storage.");
		}
		return;
	}

	if (strcmp(message, "purge") == 0) {
		if (!IsAdmin(c, player_name)) {
			RequestSendMail(c, player_name, "Unauthorized", "Admin only.");
		} else {
			reset_bank();
			RequestSendMail(c, player_name, "Purge", "All bank data purged.");
		}
		return;
	}

	if (strcmp(message, "abort") == 0) {
		StopTransferCommand(c, player_name);
		return;
	}

	if (strncmp(message, "yell ", 5) == 0) {
		ChatCommand(c, player_name, message + 5, COMMAND_CHANNEL_WORLD);
		return;
	}

	if (strcmp(message, "quest") == 0) {
		RequestSendMail(c, player_name, "Quest", "Not implemented: requires quest protocol.");
		return;
	}

	if (strcmp(message, "guild") == 0) {
		if (!IsAdmin(c, player_name)) {
			RequestSendMail(c, player_name, "Unauthorized", "Admin only.");
		} else {
			RequestAllianceMemberInfo(c);
			RequestSendMail(c, player_name, "Guild", "Alliance member info requested.");
		}
		return;
	}

	if (strcmp(message, "camp") == 0) {
		RequestSendMail(c, player_name, "Camp", "Not implemented: requires camp/encampment protocol.");
		return;
	}

	if (strcmp(message, "campleader") == 0) {
		RequestSendMail(c, player_name, "Camp Leader", "Not implemented: requires camp leader protocol.");
		return;
	}


	if (strcmp(message, "snowbeast") == 0) {
		RequestSendMail(c, player_name, "Snow Beast", "Not implemented: requires beast event protocol.");
		return;
	}

	if (strcmp(message, "stop") == 0) {
		StopTransferCommand(c, player_name);
		return;
	}

	if (strcmp(message, "reloadacc") == 0) {
		if (!IsAdmin(c, player_name)) {
			RequestSendMail(c, player_name, "Unauthorized", "Admin only.");
		} else {
			RequestSendMail(c, player_name, "Reload Acc", "Not implemented: requires account re-login.");
		}
		return;
	}

	if (strcmp(message, "members") == 0) {
		RequestAllianceMemberInfo(c);
		RequestSendMail(c, player_name, "Members", "Alliance member list requested. Check console.");
		return;
	}

	if (strcmp(message, "busrank") == 0) {
		RequestSendMail(c, player_name, "Bus Rank", "Not implemented: requires bus/shuttle protocol.");
		return;
	}

	if (strcmp(message, "resetstats") == 0) {
		if (!IsAdmin(c, player_name)) {
			RequestSendMail(c, player_name, "Unauthorized", "Admin only.");
		} else {
			reset_bank();
			RequestSendMail(c, player_name, "Reset Stats", "Bank statistics reset.");
		}
		return;
	}

	if (strcmp(message, "joingvg") == 0) {
		RequestSendMail(c, player_name, "Join GVG", "Not implemented: requires GVG protocol.");
		return;
	}

	if (strcmp(message, "leavegvg") == 0) {
		RequestSendMail(c, player_name, "Leave GVG", "Not implemented: requires GVG protocol.");
		return;
	}

	if (strcmp(message, "joinca") == 0) {
		RequestSendMail(c, player_name, "Join CA", "Not implemented: requires CA protocol.");
		return;
	}

	if (strcmp(message, "leaveca") == 0) {
		RequestSendMail(c, player_name, "Leave CA", "Not implemented: requires CA protocol.");
		return;
	}

	if (strcmp(message, "joinda") == 0) {
		RequestSendMail(c, player_name, "Join DA", "Not implemented: requires DA protocol.");
		return;
	}

	if (strcmp(message, "leaveda") == 0) {
		RequestSendMail(c, player_name, "Leave DA", "Not implemented: requires DA protocol.");
		return;
	}

	/* ---- Search commands ---- */
	if (strncmp(message, "findtile ", 9) == 0) {
		char rest[128];
		strncpy(rest, message + 9, sizeof(rest) - 1);
		rest[sizeof(rest) - 1] = '\0';
		char word[32] = {0};
		int level = 0;
		int n = sscanf(rest, "%31s %d", word, &level);
		if (n >= 1) {
			int kind = GatherResourceKind(word);
			if (n == 1) level = 0;
			char text[512];
			int found = GatheringFindTiles(c, kind, level, text, sizeof(text));
			RequestSendMailFmt(c, player_name, "Find Tile",
				found ? "%d tile(s):\n%s" : "No matching tiles known yet.", found, text);
		} else {
			RequestSendMail(c, player_name, "Find Tile", "Usage: findtile <type> [level]");
		}
		return;
	}

	if (strncmp(message, "findmonster ", 12) == 0) {
		RequestSendMail(c, player_name, "Find Monster", "Not implemented: requires monster map scan.");
		return;
	}

	if (strncmp(message, "findnest ", 9) == 0) {
		RequestSendMail(c, player_name, "Find Nest", "Not implemented: requires nest map scan.");
		return;
	}

	/* ---- Balance commands ---- */
	if (strcmp(message, "adminbal") == 0) {
		if (!IsAdmin(c, player_name)) {
			RequestSendMail(c, player_name, "Unauthorized", "Admin only.");
		} else {
			ShowBankBalance(c, player_name);
		}
		return;
	}

	if (strcmp(message, "adminbag") == 0) {
		if (!IsAdmin(c, player_name)) {
			RequestSendMail(c, player_name, "Unauthorized", "Admin only.");
		} else {
			SendBankBag(c, player_name);
		}
		return;
	}

	if (strncmp(message, "setbal ", 7) == 0) {
		if (!IsAdmin(c, player_name)) {
			RequestSendMail(c, player_name, "Unauthorized", "Admin only.");
		} else {
			AdminLedgerCommand(c, player_name, message + 7, 0);
		}
		return;
	}

	if (strncmp(message, "addbal ", 7) == 0) {
		if (!IsAdmin(c, player_name)) {
			RequestSendMail(c, player_name, "Unauthorized", "Admin only.");
		} else {
			AdminLedgerCommand(c, player_name, message + 7, 1);
		}
		return;
	}

	if (strncmp(message, "setacc ", 7) == 0) {
		if (!IsAdmin(c, player_name)) {
			RequestSendMail(c, player_name, "Unauthorized", "Admin only.");
		} else {
			char target[64], type[16], amount_str[32];
			if (sscanf(message + 7, "%63s %15s %31s", target, type, amount_str) == 3) {
				set_balance(target, type, (long)parse_number_u64(amount_str));
				char text[256];
				get_balance_text(target, text);
				RequestSendMail(c, player_name, "Set Account", text);
			} else {
				RequestSendMail(c, player_name, "Set Account", "Usage: setacc <player> <resource> <amount>");
			}
		}
		return;
	}

	if (strncmp(message, "transfer ", 9) == 0) {
		if (!IsAdmin(c, player_name)) {
			RequestSendMail(c, player_name, "Unauthorized", "Admin only.");
		} else {
			char from[64], to[64], type[16], amount_str[32];
			if (sscanf(message + 9, "%63s %63s %15s %31s", from, to, type, amount_str) == 4) {
				transfer_balance(from, to, type, (long)parse_number_u64(amount_str));
				char text[256];
				get_balance_text(to, text);
				RequestSendMail(c, player_name, "Transfer", text);
			} else {
				RequestSendMail(c, player_name, "Transfer", "Usage: transfer <from> <to> <resource> <amount>");
			}
		}
		return;
	}

	if (strncmp(message, "setrsslimit ", 12) == 0) {
		if (!IsAdmin(c, player_name)) {
			RequestSendMail(c, player_name, "Unauthorized", "Admin only.");
		} else {
			char target[64], amount_str[32];
			if (sscanf(message + 12, "%63s %31s", target, amount_str) == 2) {
				set_balance(target, "rsslimit", (long)parse_number_u64(amount_str));
				char text[256];
				get_balance_text(target, text);
				RequestSendMail(c, player_name, "Set RSS Limit", text);
			} else {
				RequestSendMail(c, player_name, "Set RSS Limit", "Usage: setrsslimit <player> <amount>");
			}
		}
		return;
	}

	/* ---- Resource commands ---- */
	if (strncmp(message, "donate ", 7) == 0) {
		char target[64], type[16], amount_str[32];
		if (sscanf(message + 7, "%63s %15s %31s", target, type, amount_str) == 3) {
			add_balance(target, type, (long)parse_number_u64(amount_str));
			char text[256];
			get_balance_text(target, text);
			RequestSendMail(c, player_name, "Donate", text);
		} else {
			RequestSendMail(c, player_name, "Donate", "Usage: donate <player> <resource> <amount>");
		}
		return;
	}

	if (strncmp(message, "admindonate ", 12) == 0) {
		if (!IsAdmin(c, player_name)) {
			RequestSendMail(c, player_name, "Unauthorized", "Admin only.");
		} else {
			char target[64], type[16], amount_str[32];
			if (sscanf(message + 12, "%63s %15s %31s", target, type, amount_str) == 3) {
				add_balance(target, type, (long)parse_number_u64(amount_str));
				char text[256];
				get_balance_text(target, text);
				RequestSendMail(c, player_name, "Admin Donate", text);
			} else {
				RequestSendMail(c, player_name, "Admin Donate", "Usage: admindonate <player> <resource> <amount>");
			}
		}
		return;
	}

	if (strcmp(message, "adminrss") == 0) {
		if (!IsAdmin(c, player_name)) {
			RequestSendMail(c, player_name, "Unauthorized", "Admin only.");
		} else {
			char text[512];
			get_bank_total(text);
			RequestSendMail(c, player_name, "Admin RSS", text);
		}
		return;
	}

}

uint64_t parse_number_u64(const char *str) {
    double value = 0.0;
    char suffix = '\0';

    // Read numeric part and optional suffix
    sscanf(str, "%lf%c", &value, &suffix);
    suffix = tolower(suffix); // handle both lowercase and uppercase

    // Apply multiplier
    switch (suffix) {
        case 'k': value *= 1000ULL; break;
        case 'm': value *= 1000000ULL; break;
        case 'b': value *= 1000000000ULL; break;
        default: break;//return 0;//break; // no suffix
    }

    if (value < 0) value = 0;

    return (uint64_t)value;
}


static void ResourceCommandHandler(
    Connection *c,
    const char *player_name,
    const char *message,
    ResourceType type,
    const char *name
)
{
	
	if (c->transfer.state != TRANSFER_IDLE) {
		// Same player -> replace current pending request 
		if (strcmp(c->transfer.target_name, player_name) == 0) {
			memset(&c->transfer, 0, sizeof(c->transfer));
			c->transfer.state = TRANSFER_IDLE;
			// Continue below and assign the new request 
		} else {
			// Different player -> reject
			RequestSendMailFmt(
				c,
				player_name,
				"Transfer Busy",
				"Currently sending resources to %s. Use %cstop to cancel.",
				c->transfer.target_name,
				c->bot.command_prefix
			);
			
			return;
		}
	}
	
	char amount_str[32] = {0};
	
	if (sscanf(message, "%31s", amount_str) != 1)
		return;
	
	uint64_t amount = parse_number_u64(amount_str);
	
	if (amount == 0 || amount > UINT32_MAX)
		return;
	
	uint32_t current = 0;
	uint32_t reserve = 0;
	
	switch (type) {
		case RESOURCE_FOOD:
			current = c->resources.food;
			reserve = c->bank.reserve.food;
			break;
		case RESOURCE_ROCK:
			current = c->resources.rock;
			reserve = c->bank.reserve.rock;
			break;
		case RESOURCE_WOOD:
			current = c->resources.wood;
			reserve = c->bank.reserve.wood;
			break;
		case RESOURCE_ORE:
			current = c->resources.ore;
			reserve = c->bank.reserve.ore;
			break;
		case RESOURCE_GOLD:
			current = c->resources.gold;
			reserve = c->bank.reserve.gold;
			break;
	}
	
	if (current <= reserve || amount > (current - reserve)) {
		char curr_amount_str[20];
		
		format_number2(current > reserve ? current - reserve : 0, curr_amount_str, sizeof(curr_amount_str));
		
		RequestSendMailFmt(
			c,
			player_name,
			"Not Enough Resources",
			"Only %s %s available.",
			curr_amount_str,
			name
		);
		return;
	}
	
	c->transfer.amount = (uint32_t)amount;
	c->transfer.remaining = (uint32_t)amount;
	c->transfer.resource_type = type;
	
	strcpy(c->transfer.target_name, player_name);
	
	c->transfer.state = TRANSFER_FIND_TARGET;
}



uint64_t GetBagFood(Connection *c) {
	return 
		((uint64_t)c->items[FOOD_5K].quantity * 5000) + // FOOD 5K
		((uint64_t)c->items[FOOD_30K].quantity * 30000) + // FOOD 30K
		((uint64_t)c->items[FOOD_150K].quantity * 150000) + // FOOD 150K
		((uint64_t)c->items[FOOD_500K].quantity * 500000) + // FOOD 500K
		((uint64_t)c->items[FOOD_2M].quantity * 2000000) + // FOOD 2M
		((uint64_t)c->items[FOOD_6M].quantity * 6000000) + // FOOD 6M
		((uint64_t)c->items[FOOD_20M].quantity * 20000000) + // FOOD 20M
		((uint64_t)c->items[FOOD_60M].quantity * 60000000); // FOOD 60M
}


uint64_t GetBagRock(Connection *c) {
	return 
		((uint64_t)c->items[STONE_3K].quantity   * 3000) + // STONE 3K
		((uint64_t)c->items[STONE_10K].quantity  * 10000) + // STONE 10K
		((uint64_t)c->items[STONE_50K].quantity  * 50000) + // STONE 50K
		((uint64_t)c->items[STONE_150K].quantity * 150000) + // STONE 150K
		((uint64_t)c->items[STONE_500K].quantity * 500000) + // STONE 500K
		((uint64_t)c->items[STONE_1_5M].quantity * 1500000) + // STONE 1.5M
		((uint64_t)c->items[STONE_5M].quantity   * 5000000) + // STONE 5M
		((uint64_t)c->items[STONE_15M].quantity  * 15000000); // STONE 15M
}


uint64_t GetBagWood(Connection *c) {
	return 
		((uint64_t)c->items[TIMBER_3K].quantity * 3000) + // WOOD 3K
		((uint64_t)c->items[TIMBER_10K].quantity * 10000) + // WOOD 10K
		((uint64_t)c->items[TIMBER_50K].quantity * 50000) + // WOOD 50K
		((uint64_t)c->items[TIMBER_150K].quantity * 150000) + // WOOD 150K
		((uint64_t)c->items[TIMBER_500K].quantity * 500000) + // WOOD 500K
		((uint64_t)c->items[TIMBER_1_5M].quantity * 1500000) + // WOOD 1.5M
		((uint64_t)c->items[TIMBER_5M].quantity * 5000000) + // WOOD 5M
		((uint64_t)c->items[TIMBER_15M].quantity * 15000000); // WOOD 15M
}

uint64_t GetBagOre(Connection *c) {
	return 
		((uint64_t)c->items[ORE_3K].quantity * 3000) + // ORE 3K
		((uint64_t)c->items[ORE_10K].quantity * 10000) + // ORE 10K
		((uint64_t)c->items[ORE_50K].quantity * 50000) + // ORE 50K
		((uint64_t)c->items[ORE_150K].quantity * 150000) + // ORE 150K
		((uint64_t)c->items[ORE_500K].quantity * 500000) + // ORE 500K
		((uint64_t)c->items[ORE_1_5M].quantity * 1500000) + // ORE 1.5M
		((uint64_t)c->items[ORE_5M].quantity * 5000000) + // ORE 5M
		((uint64_t)c->items[ORE_15M].quantity * 15000000); // ORE 15M
}

uint64_t GetBagGold(Connection *c) {
	return 
		((uint64_t)c->items[GOLD_3K].quantity   * 3000) + // FOOD 5K
		((uint64_t)c->items[GOLD_15K].quantity  * 15000) + // FOOD 30K
		((uint64_t)c->items[GOLD_50K].quantity  * 50000) + // FOOD 150K
		((uint64_t)c->items[GOLD_200K].quantity * 200000) + // FOOD 500K
		((uint64_t)c->items[GOLD_600K].quantity * 600000) + // FOOD 2M
		((uint64_t)c->items[GOLD_2M].quantity   * 2000000) + // FOOD 6M
		((uint64_t)c->items[GOLD_6M].quantity   * 6000000); // FOOD 20M
}


void ShowBankBalance(Connection *c, const char *player_name) {
	printf("[DEBUG] Player name: '%s'\n", player_name);
    printf("[DEBUG] Admin name : '%s'\n", c->bot.admin_name);
	if (strcmp(c->bot.admin_name, player_name) != 0) {
		// Return message if necessary 
		RequestSendMail(c, player_name, "Unauthorize", "You don't have permission to see bank balance!.");
		return;
	}
	
	if (!c->items_loaded) {
		RequestSendMail(c, player_name, "Problem encounter", "Something went wrong please try again later.");
		return;
	}
	
	
	char bank_food[20];
	char bank_rock[20];
	char bank_wood[20];
	char bank_ore [20];
	char bank_gold[20];
	
	char bag_food[20];
	char bag_rock[20];
	char bag_wood[20];
	char bag_ore [20];
	char bag_gold[20];
	
	char sum_food[20];
	char sum_rock[20];
	char sum_wood[20];
	char sum_ore [20];
	char sum_gold[20];
	
	
	// Format BANK
	format_number2(c->resources.food, bank_food, sizeof(bank_food));
	format_number2(c->resources.rock, bank_rock, sizeof(bank_rock));
	format_number2(c->resources.wood, bank_wood, sizeof(bank_wood));
	format_number2(c->resources.ore,  bank_ore,  sizeof(bank_ore));
	format_number2(c->resources.gold, bank_gold, sizeof(bank_gold));
	
	uint64_t BagFood = GetBagFood(c);
	uint64_t BagRock = GetBagRock(c);
	uint64_t BagWood = GetBagWood(c);
	uint64_t BagOre = GetBagOre(c);
	uint64_t BagGold= GetBagGold(c);
	
	// Format BAG
	format_number2(BagFood, bag_food, sizeof(bag_food));
	format_number2(BagRock, bag_rock, sizeof(bag_rock));
	format_number2(BagWood, bag_wood, sizeof(bag_wood));
	format_number2(BagOre,  bag_ore,  sizeof(bag_ore));
	format_number2(BagGold, bag_gold, sizeof(bag_gold));
		
	// Format TOTAL
	format_number2(c->resources.food + BagFood,  sum_food, sizeof(sum_food));
	format_number2(c->resources.rock + BagRock,  sum_rock, sizeof(sum_rock));
	format_number2(c->resources.wood + BagWood,  sum_wood, sizeof(sum_wood));
	format_number2(c->resources.ore  + BagOre,   sum_ore,  sizeof(sum_ore));
	format_number2(c->resources.gold + BagGold,  sum_gold, sizeof(sum_gold));
	
	RequestSendMailFmt(c, player_name, "Bank Balance", 
		"[BNK] Food: %s | Stone: %s | Wood: %s | Ore: %s | Gold: %s\n"
		"[BAG] Food: %s | Stone: %s | Wood: %s | Ore: %s | Gold: %s\n"
		"[SUM] Food: %s | Stone: %s | Wood: %s | Ore: %s | Gold: %s",
		bank_food, bank_rock, bank_wood, bank_ore, bank_gold,
		bag_food, bag_rock, bag_wood, bag_ore, bag_gold,
		sum_food, sum_rock, sum_wood, sum_ore, sum_gold
	);
	return;
}