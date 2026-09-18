/*
 * Configuration parser.
 *
 * Each configuration key is matched using a simple if/return chain.
 *
 * Although a lookup table or hash map could also be used, this function is
 * only executed once during application startup when the configuration file
 * is loaded. It is never called from the main game loop, network packet
 * processing, or any performance-critical code.
 *
 * Because the parser runs only once, the cost of multiple strcmp() calls is
 * negligible compared to the overall initialization time. This approach keeps
 * the code straightforward, easy to read, and simple to extend—adding a new
 * configuration option only requires adding another comparison.
 *
 * Prefer clarity and maintainability here over unnecessary optimization.
 */

#include "config.h"
#include <stdlib.h>

#include <ctype.h>

static CommandChannel ParseCommandChannel(const char *value)
{
    if (strcmp(value, "WORLD") == 0)
        return COMMAND_CHANNEL_WORLD;

    if (strcmp(value, "GUILD") == 0)
        return COMMAND_CHANNEL_GUILD;

    if (strcmp(value, "MAIL") == 0)
        return COMMAND_CHANNEL_MAIL;

    return COMMAND_CHANNEL_GUILD; // default
}


static uint16_t ParseShield(const char *value)
{
    if (strcmp(value, "SHIELD_4H") == 0)
        return SHIELD_4H;

    if (strcmp(value, "SHIELD_8H") == 0)
        return SHIELD_8H;

    if (strcmp(value, "SHIELD_12H") == 0)
        return SHIELD_12H;

    if (strcmp(value, "SHIELD_1D") == 0)
        return SHIELD_1D;

    if (strcmp(value, "SHIELD_3D") == 0)
        return SHIELD_3D;

    if (strcmp(value, "SHIELD_7D") == 0)
        return SHIELD_7D;

    if (strcmp(value, "SHIELD_14D") == 0)
        return SHIELD_14D;

    return 0;
}

static bool ParseShieldPriority(Connection *c, const char *value)
{
    char buffer[512];

    strncpy(buffer, value, sizeof(buffer));
    buffer[sizeof(buffer) - 1] = '\0';

    int count = 0;

    char *token = strtok(buffer, ",");

    while (token && count < 8) {
        while (*token == ' ')
            token++;

        uint16_t shield = ParseShield(token);

        if (shield == 0) {
            printf("Invalid shield priority value: %s\n", token);
            return false;
        }

        c->protection.shield_priority[count++] = shield;

        token = strtok(NULL, ",");
    }

    c->protection.shield_priority_count = count;
    return true;
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

/*
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
*/

uint64_t parse_number_u64(const char *str);

static bool ParserConfig(Connection *c, const char *key, const char *value) {
	// gateway server 
	if (strcmp(key, "server.addr") == 0) {
		strncpy(c->gateway_server.addr, value, 16);
		return true;
	}
	
	if (strcmp(key, "server.port") == 0) {
		c->gateway_server.port = (uint16_t)strtoul(value, NULL, 10);
		return true;
	}
	
	// client version 
	if (strcmp(key, "client.version_major") == 0) {
		c->app.version_major = (uint8_t)strtoul(value, NULL, 10);
		return true;
	}
	
	if (strcmp(key, "client.version_minor") == 0) {
		c->app.version_minor = (uint8_t)strtoul(value, NULL, 10);
		return true;
	}
	
	if (strcmp(key, "client.version_patch") == 0) {
		c->app.version_patch = (uint16_t)strtoul(value, NULL, 10);
		return true;
	}
	
	if (strcmp(key, "client.language_code") == 0) {
		c->app.language_code = (uint8_t)strtoul(value, NULL, 10);
		return true;
	}
	
	// login 
	if (strcmp(key, "account.igg_id") == 0) {
		c->auth.igg_id = (uint64_t)strtoull(value, NULL, 10);
		return true;
	}
	
	if (strcmp(key, "account.device_uuid") == 0) {
		strncpy(c->auth.device_uuid, value, sizeof(c->auth.device_uuid));
		return true;
	}
	
	if (strcmp(key, "account.access_key") == 0) {
		strcpy(c->auth.session, value);
		c->auth.session_len = (uint16_t)strlen(c->auth.session);
		return true;
	}
	
	// command 
	if (strcmp(key, "command.input") == 0) {
		c->bot.command_input = ParseCommandChannel(value);
		return true;
	}
	
	if (strcmp(key, "command.output") == 0) {
		c->bot.command_output = ParseCommandChannel(value);
		return true;
	}
	
	if (strcmp(key, "command.prefix") == 0) {
		if (value[0] != '\0') {
			c->bot.command_prefix = value[0];
		}
		return true;
	}
	
	
	if (strcmp(key, "alliance.auto_help") == 0) {
		c->alliance.auto_help = (strcmp(value, "true") == 0);
		return true;
	}
	
	if (strcmp(key, "alliance.auto_open_gifts") == 0) {
		c->alliance.auto_open_gifts = (strcmp(value, "true") == 0);
		return true;
	}
	
	if (strcmp(key, "protection.enabled") == 0) {
		c->protection.enabled = (strcmp(value, "true") == 0);
		return true;
	}
	
	if (strcmp(key, "protection.shield_always_on") == 0) {
		c->protection.shield_always_on = (strcmp(value, "true") == 0);
		return true;
	}
	
	if (strcmp(key, "protection.shield_always_on") == 0) {
		c->protection.shield_always_on = (strcmp(value, "true") == 0);
		return true;
	}
	
	if (strcmp(key, "protection.shield_on_incoming_attack") == 0) {
		c->protection.shield_on_incoming_attack = (strcmp(value, "true") == 0);
		return true;
	}
	
	if (strcmp(key, "protection.shield_on_incoming_scout") == 0) {
		c->protection.shield_on_incoming_scout = (strcmp(value, "true") == 0);
		return true;
	}
	
	if (strcmp(key, "protection.shield_priority") == 0) {
		return ParseShieldPriority(c, value);
	}
	
	if (strcmp(key, "admin.name") == 0) {
		strncpy(c->bot.admin_name, value, sizeof(c->bot.admin_name) - 1);
		c->bot.admin_name[sizeof(c->bot.admin_name) - 1] = '\0';
		return true;
	}
	
	// Cargo ship setting 
	if (strcmp(key, "cargo_ship.auto_trade") == 0) {
		if (strcmp(value, "true") != 0 && strcmp(value, "false") != 0) {
			return false;
		}
		
		c->market.settings.auto_trade = (strcmp(value, "true") == 0);
		return true;
	}
	
	// Spend resources 
	if (strcmp(key, "cargo_ship.spend_food") == 0) {
		c->market.settings.spend_food = (strcmp(value, "true") == 0);
		return true;
	}
	
	if (strcmp(key, "cargo_ship.spend_rock") == 0) {
		c->market.settings.spend_rock = (strcmp(value, "true") == 0);
		return true;
	}
	
	if (strcmp(key, "cargo_ship.spend_wood") == 0) {
		c->market.settings.spend_wood = (strcmp(value, "true") == 0);
		return true;
	}
	
	if (strcmp(key, "cargo_ship.spend_ore") == 0) {
		c->market.settings.spend_ore  = (strcmp(value, "true") == 0);
		return true;
	}
	
	if (strcmp(key, "cargo_ship.spend_gold") == 0) {
		c->market.settings.spend_gold  = (strcmp(value, "true") == 0);
		return true;
	}
	
	// Use resources from bag
	if (strcmp(key, "cargo_ship.use_bag_rss") == 0) {
		c->market.settings.use_bag_rss  = (strcmp(value, "true") == 0);
		return true;
	}
	
	
	// Reserved resources 
	if (strcmp(key, "cargo_ship.reserve_food") == 0) {
		c->market.reserve.food  = (uint32_t)parse_number_u64(value);
		// printf("cargo_ship.reserve_food: %lu\n", c->market.reserve.food);
		return true;
	}
	
	if (strcmp(key, "cargo_ship.reserve_rock") == 0) {
		c->market.reserve.rock  = (uint32_t)parse_number_u64(value);
		return true;
	}
	
	if (strcmp(key, "cargo_ship.reserve_wood") == 0) {
		c->market.reserve.wood  = (uint32_t)parse_number_u64(value);
		return true;
	}
	
	if (strcmp(key, "cargo_ship.reserve_ore") == 0) {
		c->market.reserve.ore  = (uint32_t)parse_number_u64(value);
		return true;
	}
	
	if (strcmp(key, "cargo_ship.reserve_gold") == 0) {
		c->market.reserve.gold  = (uint32_t)parse_number_u64(value);
		return true;
	}

	/* Automation master/domain switches. */
	if (strcmp(key, "automation.enabled") == 0) { c->automation.enabled = (strcmp(value, "true") == 0); return true; }
	if (strcmp(key, "automation.building") == 0) { c->automation.building = (strcmp(value, "true") == 0); return true; }
	if (strcmp(key, "automation.research") == 0) { c->automation.research = (strcmp(value, "true") == 0); return true; }
	if (strcmp(key, "automation.training") == 0) { c->automation.training = (strcmp(value, "true") == 0); return true; }
	if (strcmp(key, "automation.traps") == 0) { c->automation.traps = (strcmp(value, "true") == 0); return true; }
	if (strcmp(key, "automation.hospital") == 0) { c->automation.hospital = (strcmp(value, "true") == 0); return true; }
	if (strcmp(key, "automation.hunting") == 0) { c->automation.hunting = (strcmp(value, "true") == 0); return true; }
	if (strcmp(key, "automation.quests") == 0) { c->automation.quests = (strcmp(value, "true") == 0); return true; }
	if (strcmp(key, "automation.activities") == 0) { c->automation.activities = (strcmp(value, "true") == 0); return true; }
	if (strcmp(key, "automation.alliance") == 0) { c->automation.alliance = (strcmp(value, "true") == 0); return true; }
	if (strcmp(key, "automation.mail") == 0) { c->automation.mail = (strcmp(value, "true") == 0); return true; }
	if (strcmp(key, "automation.economy") == 0) { c->automation.economy = (strcmp(value, "true") == 0); return true; }
	if (strcmp(key, "automation.map") == 0) { c->automation.map = (strcmp(value, "true") == 0); return true; }
	if (strcmp(key, "automation.war") == 0) { c->automation.war = (strcmp(value, "true") == 0); return true; }
	/* New automation subsystems from dump */
	if (strcmp(key, "automation.daily_mission") == 0) { c->automation.daily_mission = (strcmp(value, "true") == 0); return true; }
	if (strcmp(key, "automation.vip_mission") == 0) { c->automation.vip_mission = (strcmp(value, "true") == 0); return true; }
	if (strcmp(key, "automation.daily_signin") == 0) { c->automation.daily_signin = (strcmp(value, "true") == 0); return true; }
	if (strcmp(key, "automation.pet_training") == 0) { c->automation.pet_training = (strcmp(value, "true") == 0); return true; }
	if (strcmp(key, "automation.hero_system") == 0) { c->automation.hero_system = (strcmp(value, "true") == 0); return true; }
	if (strcmp(key, "automation.item_craft") == 0) { c->automation.item_craft = (strcmp(value, "true") == 0); return true; }
	if (strcmp(key, "automation.relics") == 0) { c->automation.relics = (strcmp(value, "true") == 0); return true; }
	if (strcmp(key, "automation.lucky_card") == 0) { c->automation.lucky_card = (strcmp(value, "true") == 0); return true; }
	if (strcmp(key, "automation.dark_nest") == 0) { c->automation.dark_nest = (strcmp(value, "true") == 0); return true; }
	if (strcmp(key, "automation.achievement") == 0) { c->automation.achievement = (strcmp(value, "true") == 0); return true; }
	if (strcmp(key, "automation.quest_chapter") == 0) { c->automation.quest_chapter = (strcmp(value, "true") == 0); return true; }

    if (strcmp(key, "building.max_level") == 0) { c->automation.building_max_level = (uint8_t)strtoul(value, NULL, 10); return true; }
    if (strcmp(key, "building.lowest_level_first") == 0) { c->automation.building_lowest_level_first = (strcmp(value, "true") == 0); return true; }

    if (strcmp(key, "research.auto_start") == 0) { c->automation.research_auto_start = (strcmp(value, "true") == 0); return true; }
    if (strcmp(key, "research.complete_free") == 0) { c->automation.research_auto_complete_free = (strcmp(value, "true") == 0); return true; }
    if (strcmp(key, "research.instant") == 0) { c->automation.research_auto_instant = (strcmp(value, "true") == 0); return true; }
    if (strcmp(key, "research.max_level") == 0) { c->automation.research_max_level = (uint8_t)strtoul(value, NULL, 10); return true; }
    if (strcmp(key, "research.priority") == 0) {
        char buf[512]; strncpy(buf, value, sizeof(buf)-1); buf[sizeof(buf)-1] = 0;
        c->automation.research_priority_count = 0;
        char *tok = strtok(buf, ",");
        while (tok && c->automation.research_priority_count < 32) {
            while (*tok == ' ' || *tok == '\t') ++tok;
            unsigned long id = strtoul(tok, NULL, 10);
            if (id > 0 && id <= 400) c->automation.research_priority[c->automation.research_priority_count++] = (uint16_t)id;
            tok = strtok(NULL, ",");
        }
        return true;
    }

    if (strcmp(key, "training.auto_start") == 0) { c->automation.training_auto_start = (strcmp(value, "true") == 0); return true; }
    if (strcmp(key, "training.kind") == 0) { c->automation.training_kind = (uint8_t)strtoul(value, NULL, 10); return true; }
    if (strcmp(key, "training.tier") == 0) { c->automation.training_tier = (uint8_t)strtoul(value, NULL, 10); return true; }
    if (strcmp(key, "training.batch") == 0) { c->automation.training_batch = (uint32_t)strtoul(value, NULL, 10); return true; }
    if (strcmp(key, "training.highest_tier") == 0) { c->automation.training_highest_tier = (strcmp(value, "true") == 0); return true; }

    /* Bag-resource top-up (use_bag_rss): when the account's on-hand stock is
       below what a build/research/train needs, spend a matching resource pack
       from the bag to cover the deficit instead of letting the server reject.
       rss_floor is the per-resource minimum (k/m/b suffixes allowed) kept for
       building and research. */
    if (strcmp(key, "automation.use_bag_rss") == 0) { c->automation.use_bag_rss = (strcmp(value, "true") == 0); return true; }
    if (strcmp(key, "automation.rss_floor") == 0) { c->automation.rss_floor = (uint32_t)parse_number_u64(value); return true; }

    if (strcmp(key, "hospital.auto_heal") == 0) { c->automation.hospital_auto_heal = (strcmp(value, "true") == 0); return true; }
    if (strcmp(key, "hospital.instant_heal") == 0) { c->automation.hospital_instant_heal = (strcmp(value, "true") == 0); return true; }

	// Gathering settings
	// "gathering" is a convenient alias for the master switch.
	if (strcmp(key, "gathering") == 0 || strcmp(key, "gathering.enabled") == 0) {
		c->gathering.enabled = (strcmp(value, "true") == 0);
		return true;
	}

	if (strcmp(key, "gathering.spare_army") == 0) {
		c->gathering.spare_army = (strcmp(value, "true") == 0);
		return true;
	}

	if (strcmp(key, "gathering.highest_level_first") == 0) {
		c->gathering.highest_level_first = (strcmp(value, "true") == 0);
		return true;
	}

	if (strcmp(key, "gathering.low_tier_first") == 0) {
		c->gathering.low_tier_first = (strcmp(value, "true") == 0);
		return true;
	}

	if (strcmp(key, "gathering.lowest_resource") == 0) {
		c->gathering.lowest_resource = (strcmp(value, "true") == 0);
		return true;
	}

	if (strcmp(key, "gathering.clearable_only") == 0) {
		c->gathering.clearable_only = (strcmp(value, "true") == 0);
		return true;
	}

	if (strcmp(key, "gathering.gems_ignore_level") == 0) {
		c->gathering.gems_ignore_level = (strcmp(value, "true") == 0);
		return true;
	}

	if (strcmp(key, "gathering.recall_camps") == 0) {
		c->gathering.recall_camps = (strcmp(value, "true") == 0);
		return true;
	}

	if (strcmp(key, "gathering.gathering_gear") == 0) {
		c->gathering.gathering_gear = (strcmp(value, "true") == 0);
		return true;
	}

	if (strcmp(key, "gathering.max_armies") == 0) {
		c->gathering.max_armies = (uint8_t)strtoul(value, NULL, 10);
		return true;
	}

	if (strcmp(key, "gathering.delay_seconds") == 0) {
		c->gathering.delay_seconds = (uint16_t)strtoul(value, NULL, 10);
		return true;
	}

	if (strcmp(key, "gathering.max_travel_seconds") == 0) {
		c->gathering.max_travel_seconds = (uint32_t)strtoul(value, NULL, 10);
		return true;
	}

	if (strcmp(key, "gathering.search_multiplier") == 0) {
		c->gathering.search_multiplier = (uint8_t)strtoul(value, NULL, 10);
		return true;
	}

	if (strcmp(key, "gathering.min_gather_distance") == 0) {
		c->gathering.min_gather_distance = (uint32_t)strtoul(value, NULL, 10);
		return true;
	}

	if (strcmp(key, "gathering.min_south_units") == 0) {
		c->gathering.min_south_units = (uint32_t)strtoul(value, NULL, 10);
		return true;
	}

	if (strcmp(key, "gathering.send_2415") == 0) {
		c->gathering.send_2415 = (strcmp(value, "true") == 0);
		return true;
	}

	if (strcmp(key, "gathering.send_6615_fat") == 0) {
		c->gathering.send_6615_fat = (strcmp(value, "true") == 0);
		return true;
	}

	if (strcmp(key, "gathering.send_6615_sparse") == 0) {
		c->gathering.send_6615_sparse = (strcmp(value, "true") == 0);
		return true;
	}

	if (strcmp(key, "gathering.pin_node_id") == 0) {
		c->gathering.pin_node_id = (uint32_t)strtoul(value, NULL, 0);
		return true;
	}

	if (strcmp(key, "gathering.pin_node_id_zero") == 0) {
		c->gathering.pin_node_id_zero = (strcmp(value, "true") == 0);
		return true;
	}

	if (strcmp(key, "gathering.pin_dest_y") == 0) {
		c->gathering.pin_dest_y = (uint32_t)strtoul(value, NULL, 0);
		return true;
	}

	if (strcmp(key, "gathering.world_x_offset") == 0) {
		c->gathering.world_x_offset = (uint32_t)strtoul(value, NULL, 0);
		return true;
	}

	if (strcmp(key, "gathering.world_y_offset") == 0) {
		c->gathering.world_y_offset = (uint32_t)strtoul(value, NULL, 0);
		return true;
	}

	if (strcmp(key, "gathering.scan_reach") == 0) {
		c->gathering.scan_reach = (uint8_t)strtoul(value, NULL, 10);
		return true;
	}

	if (strcmp(key, "gathering.minimum_tile_count") == 0) {
		c->gathering.minimum_tile_count = (uint32_t)strtoul(value, NULL, 10);
		return true;
	}

	if (strcmp(key, "gathering.schedule_enabled") == 0) {
		c->gathering.schedule_enabled = (strcmp(value, "true") == 0);
		return true;
	}

	if (strcmp(key, "gathering.schedule_start") == 0) {
		c->gathering.schedule_start_min = (uint32_t)ParseClockMinutes(value);
		return true;
	}

	if (strcmp(key, "gathering.schedule_end") == 0) {
		c->gathering.schedule_end_min = (uint32_t)ParseClockMinutes(value);
		return true;
	}

	// Gathering resource types
	if (strcmp(key, "gathering.resource_food") == 0) {
		c->gathering.resource_enabled[1] = (strcmp(value, "true") == 0);
		return true;
	}

	if (strcmp(key, "gathering.resource_stone") == 0) {
		c->gathering.resource_enabled[2] = (strcmp(value, "true") == 0);
		return true;
	}

	if (strcmp(key, "gathering.resource_ore") == 0) {
		c->gathering.resource_enabled[3] = (strcmp(value, "true") == 0);
		return true;
	}

	if (strcmp(key, "gathering.resource_wood") == 0) {
		c->gathering.resource_enabled[4] = (strcmp(value, "true") == 0);
		return true;
	}

	if (strcmp(key, "gathering.resource_gold") == 0) {
		c->gathering.resource_enabled[5] = (strcmp(value, "true") == 0);
		return true;
	}

	if (strcmp(key, "gathering.resource_gems") == 0) {
		c->gathering.resource_enabled[6] = (strcmp(value, "true") == 0);
		return true;
	}

	// Gathering levels
	if (strcmp(key, "gathering.level_1") == 0) {
		c->gathering.level_enabled[1] = (strcmp(value, "true") == 0);
		return true;
	}

	if (strcmp(key, "gathering.level_2") == 0) {
		c->gathering.level_enabled[2] = (strcmp(value, "true") == 0);
		return true;
	}

	if (strcmp(key, "gathering.level_3") == 0) {
		c->gathering.level_enabled[3] = (strcmp(value, "true") == 0);
		return true;
	}

	if (strcmp(key, "gathering.level_4") == 0) {
		c->gathering.level_enabled[4] = (strcmp(value, "true") == 0);
		return true;
	}

	if (strcmp(key, "gathering.level_5") == 0) {
		c->gathering.level_enabled[5] = (strcmp(value, "true") == 0);
		return true;
	}

	if (strcmp(key, "gathering.enable_gbg") == 0) {
		c->gathering.gbg_enabled = (strcmp(value, "true") == 0);
		return true;
	}
	if (strcmp(key, "gathering.enable_chaos") == 0) {
		c->gathering.chaos_enabled = (strcmp(value, "true") == 0);
		return true;
	}

	// Gathering heroes
	for (int i = 0; i < GATHER_HERO_SLOTS; ++i) {
		char hero_key[32];
		snprintf(hero_key, sizeof(hero_key), "gathering.hero_%d", i + 1);
		if (strcmp(key, hero_key) == 0) {
			c->gathering.hero_ids[i] = (uint16_t)strtoul(value, NULL, 10);
			return true;
		}
	}

	/* Daily Mission */
	if (strcmp(key, "daily_mission.auto_claim") == 0) { c->automation.daily_mission = (strcmp(value, "true") == 0); return true; }
	if (strcmp(key, "daily_mission.auto_speedup") == 0) { c->automation.daily_mission_auto_speedup = (strcmp(value, "true") == 0); return true; }

	/* VIP Mission */
	if (strcmp(key, "vip_mission.auto_collect") == 0) { c->automation.vip_mission_auto_collect = (strcmp(value, "true") == 0); return true; }
	if (strcmp(key, "vip_mission.auto_speedup") == 0) { c->automation.vip_mission_auto_speedup = (strcmp(value, "true") == 0); return true; }

	/* Daily Sign-in */
	if (strcmp(key, "daily_signin.auto_signin") == 0) { c->automation.daily_signin_auto_signin = (strcmp(value, "true") == 0); return true; }
	if (strcmp(key, "daily_signin.auto_choose_hero") == 0) { c->automation.daily_signin_auto_choose_hero = (strcmp(value, "true") == 0); return true; }
	if (strcmp(key, "daily_signin.hero_choice") == 0) { c->automation.daily_signin_hero_choice = (uint8_t)strtoul(value, NULL, 10); return true; }

	/* Pet Training */
	if (strcmp(key, "pet_training.auto_start") == 0) { c->automation.pet_training_auto_start = (strcmp(value, "true") == 0); return true; }
	if (strcmp(key, "pet_training.auto_complete_free") == 0) { c->automation.pet_training_auto_complete_free = (strcmp(value, "true") == 0); return true; }
	if (strcmp(key, "pet_training.auto_instant") == 0) { c->automation.pet_training_auto_instant = (strcmp(value, "true") == 0); return true; }
	if (strcmp(key, "pet_training.use_speedup") == 0) { c->automation.pet_training_use_speedup = (strcmp(value, "true") == 0); return true; }
	if (strcmp(key, "pet_training.pet_id") == 0) {
		c->automation.pet_training_pet_id_count = 0;
		char *tok = strtok(value, ",");
		while (tok && c->automation.pet_training_pet_id_count < 16) {
			while (*tok == ' ' || *tok == '\t') ++tok;
			unsigned long id = strtoul(tok, NULL, 10);
			if (id > 0) c->automation.pet_training_pet_id[c->automation.pet_training_pet_id_count++] = (uint16_t)id;
			tok = strtok(NULL, ",");
		}
		return true;
	}
	if (strcmp(key, "pet_training.type") == 0) {
		c->automation.pet_training_type_count = 0;
		char *tok = strtok(value, ",");
		while (tok && c->automation.pet_training_type_count < 16) {
			while (*tok == ' ' || *tok == '\t') ++tok;
			unsigned long t = strtoul(tok, NULL, 10);
			if (t > 0) c->automation.pet_training_type[c->automation.pet_training_type_count++] = (uint8_t)t;
			tok = strtok(NULL, ",");
		}
		return true;
	}

	/* Hero System */
	if (strcmp(key, "hero_system.auto_enhance") == 0) { c->automation.hero_auto_enhance = (strcmp(value, "true") == 0); return true; }
	if (strcmp(key, "hero_system.auto_starup") == 0) { c->automation.hero_auto_starup = (strcmp(value, "true") == 0); return true; }
	if (strcmp(key, "hero_system.auto_skill") == 0) { c->automation.hero_auto_skill = (strcmp(value, "true") == 0); return true; }

	/* Item Crafting */
	if (strcmp(key, "item_craft.auto_start") == 0) { c->automation.item_craft_auto_start = (strcmp(value, "true") == 0); return true; }
	if (strcmp(key, "item_craft.auto_finish") == 0) { c->automation.item_craft_auto_finish = (strcmp(value, "true") == 0); return true; }
	if (strcmp(key, "item_craft.recipe_id") == 0) {
		c->automation.item_craft_recipe_count = 0;
		char *tok = strtok(value, ",");
		while (tok && c->automation.item_craft_recipe_count < 32) {
			while (*tok == ' ' || *tok == '\t') ++tok;
			unsigned long id = strtoul(tok, NULL, 10);
			if (id > 0) c->automation.item_craft_recipe_id[c->automation.item_craft_recipe_count++] = (uint16_t)id;
			tok = strtok(NULL, ",");
		}
		return true;
	}
	if (strcmp(key, "item_craft.count") == 0) {
		c->automation.item_craft_count_parsed = 0;
		char *tok = strtok(value, ",");
		while (tok && c->automation.item_craft_count_parsed < 32) {
			while (*tok == ' ' || *tok == '\t') ++tok;
			unsigned long cnt = strtoul(tok, NULL, 10);
			if (cnt > 0) c->automation.item_craft_count[c->automation.item_craft_count_parsed++] = (uint16_t)cnt;
			tok = strtok(NULL, ",");
		}
		return true;
	}

	/* Achievement */
	if (strcmp(key, "achievement.auto_claim") == 0) { c->automation.achievement = (strcmp(value, "true") == 0); return true; }
	/* 3179 empirical kind-byte testing.  !=0 forces the discriminator for every
	 * prize (0x12 SoloBattle / 0x0f BPDailyMission / 0x0d UISlotMgr). */
	if (strcmp(key, "achievement.kind_override") == 0) { c->automation.achievement_kind_override = (uint8_t)strtoul(value, 0, 0); return true; }
	if (strcmp(key, "claim.daily_mission_via_3179") == 0) { c->automation.daily_mission_via_3179 = (strcmp(value, "true") == 0); return true; }
	if (strcmp(key, "claim.quest_chapter_via_3179") == 0) { c->automation.quest_chapter_via_3179 = (strcmp(value, "true") == 0); return true; }

	/* Quest Chapter */
	if (strcmp(key, "quest_chapter.auto_claim") == 0) { c->automation.quest_chapter = (strcmp(value, "true") == 0); return true; }

	/* Expedition */
	if (strcmp(key, "expedition.auto_claim") == 0) { c->automation.expedition = (strcmp(value, "true") == 0); return true; }

	/* Valhalla */
	if (strcmp(key, "valhalla.auto_claim") == 0) { c->automation.valhalla = (strcmp(value, "true") == 0); return true; }

	/* Adventure */
	if (strcmp(key, "adventure.auto_claim") == 0) { c->automation.adventure = (strcmp(value, "true") == 0); return true; }
	if (strcmp(key, "adventure.auto_hunt") == 0) { c->automation.fantasy_realm_auto_hunt = (strcmp(value, "true") == 0); return true; }
	if (strcmp(key, "adventure.target_monster_id") == 0) { c->automation.fantasy_realm_target_monster_id = (uint16_t)strtoul(value, NULL, 10); return true; }

	/* Relics */
	if (strcmp(key, "relics.auto_gacha") == 0) { c->automation.relics_auto_gacha = (strcmp(value, "true") == 0); return true; }
	if (strcmp(key, "relics.auto_enhance") == 0) { c->automation.relics_auto_enhance = (strcmp(value, "true") == 0); return true; }
	if (strcmp(key, "relics.auto_synthesize") == 0) { c->automation.relics_auto_synthesize = (strcmp(value, "true") == 0); return true; }

	/* Serial Gift */
	if (strcmp(key, "serial_gift.auto_claim") == 0) { c->automation.serial_gift = (strcmp(value, "true") == 0); return true; }

	/* Old Player Back (claim-only; no purchase path) */
	if (strcmp(key, "oldplayerback.auto_claim") == 0) { c->automation.oldplayerback = (strcmp(value, "true") == 0); return true; }

	/* Gift Activity (claim-only; no purchase path) */
	if (strcmp(key, "gift_activity.auto_claim") == 0) { c->automation.gift_activity = (strcmp(value, "true") == 0); return true; }

	/* Bot activity log (local tabular journal; default ON) */
	if (strcmp(key, "activity.log") == 0) { c->automation.activity_log = (strcmp(value, "true") == 0); return true; }
	if (strcmp(key, "activity.log_path") == 0) {
		snprintf(c->automation.activity_log_path, sizeof c->automation.activity_log_path, "%s", value);
		return true;
	}

	/* Week Challenge */
	if (strcmp(key, "week_challenge.auto_claim") == 0) { c->automation.week_challenge = (strcmp(value, "true") == 0); return true; }
	if (strcmp(key, "week_challenge.auto_buy") == 0) { c->automation.week_challenge_auto_buy = (strcmp(value, "true") == 0); return true; }

	/* Lucky Card */
	if (strcmp(key, "lucky_card.auto_exchange") == 0) { c->automation.lucky_card_auto_exchange = (strcmp(value, "true") == 0); return true; }

	/* Dark Nest */
	if (strcmp(key, "dark_nest.auto_join_rally") == 0 || strcmp(key, "dark_nest.auto_rally") == 0) { c->automation.dark_nest_auto_rally = (strcmp(value, "true") == 0); return true; }

	/* Fantasy Realm */
	if (strcmp(key, "fantasy_realm.auto_hunt") == 0) { c->automation.fantasy_realm = (strcmp(value, "true") == 0); return true; }
	if (strcmp(key, "fantasy_realm.target_monster_id") == 0) { c->automation.fantasy_realm_target_monster_id = (uint16_t)strtoul(value, NULL, 10); return true; }

	/* Growth Fund */
	if (strcmp(key, "growth_fund.auto_claim") == 0) { c->automation.growth_fund = (strcmp(value, "true") == 0); return true; }

	/* Treasure Back Event */
	if (strcmp(key, "treasure_back_event.auto_claim") == 0) { c->automation.treasure_back_event = (strcmp(value, "true") == 0); return true; }

	/* Newbie Challenge */
	if (strcmp(key, "newbie_challenge.auto_claim") == 0) { c->automation.newbie_challenge = (strcmp(value, "true") == 0); return true; }

	/* Cycle Mission */
	if (strcmp(key, "cycle_mission.auto_claim") == 0) { c->automation.cycle_mission = (strcmp(value, "true") == 0); return true; }

	/* Custom Mission */
	if (strcmp(key, "custom_mission.auto_claim") == 0) { c->automation.custom_mission = (strcmp(value, "true") == 0); return true; }

	/* Mobilization (Alliance Mobilization) */
	if (strcmp(key, "mobilization.auto_claim") == 0) { c->automation.mobilization = (strcmp(value, "true") == 0); return true; }
	if (strcmp(key, "mobilization.auto_refresh") == 0) { c->automation.mobilization_auto_refresh = (strcmp(value, "true") == 0); return true; }
	if (strcmp(key, "mobilization.auto_buy") == 0) { c->automation.mobilization_auto_buy = (strcmp(value, "true") == 0); return true; }

	/* Alliance Gather Point */
	if (strcmp(key, "alliance_gather_point.auto_teleport") == 0) { c->automation.alliance_gather_point = (strcmp(value, "true") == 0); return true; }
	if (strcmp(key, "alliance_gather_point.auto_teleport_enabled") == 0) { c->automation.alliance_gather_point_auto_teleport = (strcmp(value, "true") == 0); return true; }

	/* Alliance WhiteList */
	if (strcmp(key, "alliance_whitelist.auto_accept") == 0) { c->automation.alliance_whitelist = (strcmp(value, "true") == 0); return true; }
	if (strcmp(key, "alliance_whitelist.auto_accept_enabled") == 0) { c->automation.alliance_whitelist_auto_accept = (strcmp(value, "true") == 0); return true; }

	return true;
}

bool LoadConfig(Connection *c, const char *filename)
{
	FILE *fp = fopen(filename, "r");
	
	if (!fp) {
		return false;
	}
	
	char line[512];
	char key[64], value[512];
	uint32_t line_num = 0;
	
	while (fgets(line, sizeof(line), fp)) {
		line_num++;
		
		/* Skip blank lines */
		if (line[0] == '\n' || line[0] == '\r') 
			continue;
		
		/* Skip comments */
		if (line[0] == '#' || line[0] == ';' || (line[0] == '/' && line[1] == '/')) 
			continue;
		
		if (sscanf(line, " %63[^=]= %511[^\n]", key, value) != 2) {
			line[strcspn(line, "\r\n")] = '\0';
			printf("%s:%u: error: unexpected syntax: %s\n", filename, line_num, line);
			fclose(fp);
			return false;
		}
		
		key[strcspn(key, " \t")] = '\0';
		value[strcspn(value, "\r\n")] = '\0';	
		
		if (ParserConfig(c, key, value) != true) {
			printf("%s:%u: error: `%s`\n", filename, line_num, value);
			fclose(fp);
			return false;
		}
    }

    fclose(fp);
    return true;
}