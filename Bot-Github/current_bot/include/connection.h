#ifndef CONNECTION_H
#define CONNECTION_H

#include <stdint.h>
#include <stdbool.h>

#ifdef _WIN32
  #ifndef _WIN32_WINNT
  #define _WIN32_WINNT 0x0601
  #endif
  #define WIN32_LEAN_AND_MEAN
  #define NOMINMAX
  #include <winsock2.h>
  #include <ws2tcpip.h>
  #include <windows.h>
  typedef unsigned int uint;
  #define close_socket closesocket
#else
  #include <unistd.h>
  #include <arpa/inet.h>
  #include <sys/socket.h>
  #include <netinet/in.h>
  #include <fcntl.h>
  #include <errno.h>
  #define close_socket close
#endif

#include <string.h>
#include <stdio.h>

#include "des.h"

typedef enum {
	EMS_Null,
	EMS_Begin,
	EMS_End,
	EMS_BeginAndEnd
} eMsgState;

typedef struct {
	uint16_t packet_size;
	uint16_t packet_type;
    uint8_t buffer[4096];
    size_t read_pos;
    size_t parse_pos;
} PacketStream;

typedef struct {
	uint8_t data[4096];
	uint16_t size;
	uint16_t offset;
} PacketBuffer;

/*
typedef struct {
	uint8_t data[4096];
	uint16_t size;
	uint16_t offset;
} Stream;*/


#define MAX_ITEM_COUNT 65536

typedef struct {
    uint32_t food;
    uint32_t rock;
    uint32_t wood;
    uint32_t ore;
    uint32_t gold;
} ResourceStock;

typedef struct {
    int64_t food;
    int64_t rock;
    int64_t wood;
    int64_t ore;
    int64_t gold;
} ResourceProduction;

typedef enum {
    RESOURCE_FOOD,
    RESOURCE_ROCK,
    RESOURCE_WOOD,
    RESOURCE_ORE,
    RESOURCE_GOLD
} ResourceType;

typedef struct {
	uint16_t quantity;
} Item;

typedef struct {
    uint16_t item_id;
    uint16_t item_count;
    uint8_t resource_kind;
    uint32_t resource_count;
    uint8_t rare;
} MarketItem;

typedef struct {
	bool speed_up;
    bool speed_up_research;
    bool speed_up_merging;
    bool speed_up_training;
    
    bool bright_talent_orb;
} BlackMarketBuy;

typedef struct {
	bool auto_trade;
	bool use_bag_rss;
    bool spend_food;
    bool spend_rock;
    bool spend_wood;
    bool spend_ore;
    bool spend_gold;
} MarketSettings;

typedef struct {
	bool loaded;
	bool buy_pending;
	uint8_t trade_locks;
	uint8_t trade_status;
	uint64_t refresh_time;
	MarketItem items[4];
	ResourceStock reserve;
	MarketSettings settings;
} BlackMarket;

typedef struct {
	char name[13];
	uint64_t power;
	uint64_t kills;
	uint32_t gems;
	uint32_t vip_point;
	uint16_t current_kingdom_id;
	uint16_t home_kingdom_id;
	uint16_t zone_id;
	uint8_t point_id;
	uint8_t max_marches;
	uint8_t current_marches;
	uint16_t lord_level;
} PlayerInfo;

// optional 
typedef struct {
    ResourceStock current;
    ResourceStock bag;

    bool loaded;
} ResourceInfo;

typedef struct {
    char addr[16];
    uint32_t port;
} ServerInfo;

typedef struct {
    uint8_t  version_major;
    uint8_t  version_minor;
    uint16_t version_patch;
    uint8_t  language_code;
} AppInfo;

typedef struct {
    int64_t igg_id;
    char device_uuid[50];
    uint16_t session_len;
    char session[512];
} AuthInfo;

typedef struct {
    uint32_t seq_id;
    uint32_t guest_seq_id;
} ProtocolState;

typedef struct {
    char player_name[13];
    char message[1024];
    bool pending;
} ChatState;

/*
typedef enum {
    TRANSFER_IDLE,
    TRANSFER_FIND_TARGET,
    TRANSFER_WAIT_TARGET,
    TRANSFER_SEND_MARCH,
    TRANSFER_WAIT_MARCH,
    TRANSFER_COMPLETE,
    TRANSFER_FAILED
} TransferState;


typedef struct {
	bool active;
	TransferState state;
	char request_name[13];
	char target_name[13];
	
	ResourceType resource;
	
	uint32_t total_amount;
	uint32_t remaining_amount;
	
	uint16_t zone_id;
	uint8_t point_id;
	
	uint32_t current_chunk;
} ResourceTransfer;
*/

typedef struct {
    uint32_t serial_id;

    uint64_t send_time;

    uint8_t mail_type;
    uint32_t reply_id;

    uint16_t sender_head;
    uint16_t sender_kingdom;

    char sender_tag[4];
    char sender_name[14];

    uint8_t extra_flag;

    char title[256];
    char content[4096];

    uint8_t attachment_count;

} MailInfo;

typedef enum {
    Research = 0,
    Building,
    Max,
} HelpKind;

typedef struct {
    uint32_t record_sn;
    uint16_t head;
    uint8_t rank;
    char player_name[14];

    HelpKind help_kind;

    uint16_t event_id;
    uint8_t event_data_lv;

    uint8_t already_helped;
    uint8_t help_max;
    
    uint32_t record_sn_arr[255];
} AllianceHelp;

typedef struct {
    uint16_t position_id;   /* packed tile: (pos_y << 8) | pos_x */
    uint16_t build_id;
    uint8_t level;
    uint8_t pos_x;          /* tile coordinate (new city, Vector2Int overload) */
    uint8_t pos_y;
} BuildingInfo;

/*
typedef struct {
    uint32_t serial_id;

    uint8_t status;
    uint64_t receive_time;

    uint16_t box_item_id;
    uint16_t item_id;
    uint16_t quantity;

    uint8_t item_rank;

    char sender_name[13];
} AllianceGift;
*/


typedef struct {
    uint32_t sn;
    uint8_t status;
    int64_t rcv_time;
    uint16_t box_item_id;
    uint16_t item_id;
    uint16_t num;
    uint8_t item_rank;
    // Option
    uint32_t diamond;
    uint32_t money;
    char player[13];
} AllianceGift;


#define GIFT_TABLE_SIZE 8192

typedef enum {
    GIFT_EMPTY   = 0,
    GIFT_USED    = 1,
    GIFT_DELETED = 2
} GiftSlotState;

typedef struct {
	bool loaded;
    uint16_t count;
    uint16_t index;
    bool opening;
    AllianceGift gifts[GIFT_TABLE_SIZE];
} AllianceGiftList;


typedef enum {
    GIFT_STATE_IDLE,      // No gift data loaded yet.
    GIFT_STATE_LOADING,   // Waiting for RequestAllianceGiftInfo() response.
    GIFT_STATE_READY,     // Ready to process gifts.
    GIFT_STATE_OPENING,   // Waiting for open gift response.
    GIFT_STATE_DELETING   // Waiting for delete gift response.
} GiftState;

typedef struct {
    bool auto_help;
    bool auto_open_gifts;
    uint16_t gift_count;
    uint16_t gift_offset;
    
    uint16_t unopened_gift_count;
    
    GiftState gift_state;
    uint16_t recv_index;
    AllianceGift gifts[300];
} AllianceSettings;


#define MAX_SMART_USE_ITEMS 50

typedef struct {
    uint16_t id;
    uint16_t qty;
} SmartUseItem;

typedef struct {
    uint16_t count;
    SmartUseItem items[MAX_SMART_USE_ITEMS];
} SmartUseList;

typedef enum
{
	// Token: 0x04000893 RID: 2195
	NONE,
	// Token: 0x04000894 RID: 2196
	RANK1,
	// Token: 0x04000895 RID: 2197
	RANK2,
	// Token: 0x04000896 RID: 2198
	RANK3,
	// Token: 0x04000897 RID: 2199
	RANK4,
	// Token: 0x04000898 RID: 2200
	RANK5,
	// Token: 0x04000899 RID: 2201
	RANKMAX = 5
} AllianceRank;


typedef struct {
	uint32_t Channel;
    AllianceRank Rank;
    uint8_t Apply;
    uint32_t Money;
} AllianceInfo;

typedef enum {
    HELP_SPAM_IDLE,
    HELP_SPAM_START_BUILD,
    HELP_SPAM_WAIT_BUILD,
    HELP_SPAM_WAIT_HELP,
    HELP_SPAM_CANCEL_BUILD
} HelpSpamState;

typedef enum {
    BUILD_TIMBER       = 1,
    BUILD_STONE        = 2,
    BUILD_ORE          = 3,
    BUILD_FOOD         = 4,
    BUILD_MANOR        = 5,
    BUILD_BARRACKS     = 6,
    BUILD_INFIRMARY    = 7,
    BUILD_CASTLE       = 8,
    BUILD_VAULT        = 9,
    BUILD_ACADEMY      = 10,
    BUILD_WALL         = 12,
    BUILD_WATCHTOWER   = 13,
    BUILD_EMBASSY      = 14,
    BUILD_WORKSHOP     = 15,
    BUILD_TRADING_POST = 17
} BUILDING_ID;


typedef struct {
    bool active;

    HelpSpamState state;

    uint16_t remaining_count;
    uint8_t speed;

    time_t last_action;
    
    uint16_t building_id;
    uint16_t building_pos;
    uint8_t building_level;
} HelpSpam;

typedef struct {
	bool enabled;
	
	bool send_food;
	bool send_rock;
	bool send_wood;
	bool send_ore;
	bool send_gold;
	
	ResourceStock reserve;
	uint32_t max_delivery_distance;
	
	bool use_bag_rss;
	bool use_bag_food;
	bool use_bag_rock;
	bool use_bag_wood;
	bool use_bag_ore;
	bool use_bag_gold;
} BankSettings;

typedef enum {
    COMMAND_CHANNEL_WORLD,
    COMMAND_CHANNEL_GUILD,
    COMMAND_CHANNEL_MAIL
} CommandChannel;

typedef struct {
    bool enabled;
    bool building;
    bool research;
    bool training;
    bool traps;
    bool hospital;
    bool hunting;
    bool quests;
    bool activities;
    bool alliance;
    bool mail;
    bool economy;
    bool map;
    bool war;

    /* New automation subsystems discovered from dump */
    bool daily_mission;
    bool vip_mission;
    bool daily_signin;
    bool pet_training;
    bool hero_system;
    bool item_craft;
    bool achievement;
    bool quest_chapter;
    bool expedition;
    bool valhalla;
    bool adventure;
    bool relics;
    bool serial_gift;
    bool oldplayerback;
    bool gift_activity;
    bool week_challenge;
    bool lucky_card;
    bool dark_nest;
    bool fantasy_realm;
    bool growth_fund;
    bool treasure_back_event;
    bool newbie_challenge;
    bool cycle_mission;
    bool custom_mission;
    bool mobilization;
    bool alliance_gather_point;
    bool alliance_whitelist;

    /* Queue-aware automation controls.  War is never enabled by these
       defaults; it remains an explicit opt-in switch. */
    uint8_t building_max_level;
    bool building_lowest_level_first;
    uint32_t building_last_action;
    bool building_queue_active;
    uint64_t building_finish_time;
    uint16_t building_position_id;   /* packed tile: (pos_y << 8) | pos_x */

    uint8_t  building_pos_x;         /* tile coordinate of queued/upgrading building */
    uint8_t  building_pos_y;

    /* When the server REJECTS a start-building request (RESP_BUILDINGERROR
     * 2013) the bot must not hammer the same tile forever: skip that building
     * (by its tile coordinate) until building_skip_until, and pick another. */
    uint8_t  building_skip_x;
    uint8_t  building_skip_y;
    uint32_t building_skip_until;    /* wall-clock; <now => not skipping */

    bool research_auto_start;
    bool research_auto_complete_free;
    bool research_auto_instant;
    bool research_loaded;
    uint16_t research_priority[32];
    uint8_t research_priority_count;
    uint8_t research_max_level;
    uint32_t research_last_action;
    uint32_t research_retry_time;   /* wall-clock; earliest time to resend 3202 after a rejection */
    uint8_t  research_rejects;      /* consecutive 3203 rejections received */
    /* If non-zero, the active research finished but the server never released
     * the slot (complete-free was rejected). Send 3206 CANCEL for this tech. */
    uint16_t research_cancel_pending;

    bool training_auto_start;
    uint8_t training_kind;
    uint8_t training_tier;
    uint32_t training_batch;
    uint32_t training_last_action;
    uint32_t training_bag_use_time; /* wall-clock; earliest time to use another bag food item */
    bool training_queue_active;
    uint64_t training_finish_time;

    /* When true, training picks the highest available troop tier (T4 / tier 3,
     * 0-based) instead of the literal training_tier config value. */
    bool training_highest_tier;

    /* Master switch (config: use_bag_rss).  When the account's on-hand stock is
     * below what a build/research/train needs, spend a matching resource pack
     * from the bag to cover the deficit instead of letting the server reject
     * the request.  Applied to building, research and training. */
    bool use_bag_rss;
    uint32_t rss_floor;              /* per-resource floor (count) topped up from bag */
    /* One wall-clock rate-limit slot per ResourceType so topping up food does
       not starve wood/rock within the same build/research cycle. */
    uint32_t build_bag_use_time[5];

    bool hospital_auto_heal;
    bool hospital_instant_heal;
    uint32_t hospital_last_action;
    bool hospital_queue_active;

    /* Daily Mission / Battle Pass automation */
    bool daily_mission_auto_claim;
    bool daily_mission_auto_speedup;
    uint32_t daily_mission_last_action;
    bool daily_mission_loaded;

    /* VIP Mission automation */
    bool vip_mission_auto_collect;
    bool vip_mission_auto_speedup;
    uint32_t vip_mission_last_action;
    bool vip_mission_loaded;

    /* Daily Sign-in automation */
    bool daily_signin_auto_signin;
    bool daily_signin_auto_choose_hero;
    uint8_t daily_signin_hero_choice;
    uint32_t daily_signin_last_action;
    bool daily_signin_loaded;

    /* Pet Training automation */
    bool pet_training_auto_start;
    bool pet_training_auto_complete_free;
    bool pet_training_auto_instant;
    bool pet_training_use_speedup;
    uint8_t pet_training_pet_id[5];
    uint8_t pet_training_pet_id_count;
    uint8_t pet_training_type[5];
    uint8_t pet_training_type_count;
    uint8_t pet_training_slot;
    uint32_t pet_training_last_action;
    bool pet_training_loaded;

    /* Hero System automation */
    bool hero_auto_enhance;
    bool hero_auto_starup;
    bool hero_auto_skill;
    uint32_t hero_last_action;
    bool hero_loaded;

    /* Item Crafting automation */
    bool item_craft_auto_start;
    bool item_craft_auto_finish;
    uint16_t item_craft_recipe_id[8];
    uint16_t item_craft_count[8];
    uint8_t item_craft_recipe_count;
    uint8_t item_craft_count_parsed;
    uint32_t item_craft_last_action;
    bool item_craft_loaded;

    /* Achievement automation */
    bool achievement_auto_claim;
    uint32_t achievement_last_action;
    bool achievement_loaded;
    /* 3179 kind-byte routing.  achievement_kind_override != 0 forces a fixed
     * discriminator (0x12 SoloBattle / 0x0f BPDailyMission / 0x0d UISlotMgr)
     * for every prize instead of echoing the per-activity kind the server
     * reported.  Used for A/B testing which kind the server accepts. */
    uint8_t achievement_kind_override;
    /* Route daily-mission reward (11872) and quest-chapter reward (2059)
     * through 3179 (the real-client polymorphic path) instead of the legacy
     * flat msg ids, which have no names sender.  For empirical testing. */
    bool daily_mission_via_3179;
    bool quest_chapter_via_3179;

    /* Quest Chapter automation */
    bool quest_chapter_auto_progress;
    bool quest_chapter_auto_claim;
    uint32_t quest_chapter_last_action;
    bool quest_chapter_loaded;

    /* Expedition automation */
    bool expedition_auto_prize;
    uint32_t expedition_last_action;
    bool expedition_loaded;

    /* Valhalla automation */
    bool valhalla_auto_revive;
    uint32_t valhalla_last_action;
    bool valhalla_loaded;

    /* Adventure automation */
    bool adventure_auto_hunt;
    bool adventure_auto_quest;
    uint32_t adventure_last_action;
    bool adventure_loaded;

    /* Relics automation */
    bool relics_auto_gacha;
    bool relics_auto_enhance;
    bool relics_auto_synthesize;
    uint32_t relics_last_action;
    bool relics_loaded;

    /* Serial Gift automation */
    bool serial_gift_auto_claim;
    uint32_t serial_gift_last_action;
    bool serial_gift_loaded;

    /* Old Player Back automation (claim-only) */
    bool oldplayerback_auto_claim;
    uint32_t oldplayerback_last_action;
    bool oldplayerback_loaded;

    /* Gift Activity automation (claim-only) */
    bool gift_activity_auto_claim;
    uint32_t gift_activity_last_action;
    bool gift_activity_loaded;

    /* Week Challenge automation */
    bool week_challenge_auto_buy;
    bool week_challenge_auto_prize;
    uint32_t week_challenge_last_action;
    bool week_challenge_loaded;

    /* Lucky Card automation */
    bool lucky_card_auto_exchange;
    uint32_t lucky_card_last_action;
    bool lucky_card_loaded;

    /* Dark Nest automation */
    bool dark_nest_auto_rally;
    uint32_t dark_nest_last_action;
    bool dark_nest_loaded;

    /* Fantasy Realm automation */
    bool fantasy_realm_auto_hunt;
    bool fantasy_realm_auto_summon;
    uint32_t fantasy_realm_last_action;
    bool fantasy_realm_loaded;
    uint16_t fantasy_realm_target_monster_id;

    /* Growth Fund automation */
    bool growth_fund_auto_claim;
    uint32_t growth_fund_last_action;
    bool growth_fund_loaded;

    /* Treasure Back Event automation */
    bool treasure_back_auto_claim;
    uint32_t treasure_back_last_action;
    uint32_t treasure_back_event_last_action;
    bool treasure_back_loaded;

    /* Newbie Challenge automation */
    bool newbie_challenge_auto_claim;
    uint32_t newbie_challenge_last_action;
    bool newbie_challenge_loaded;

    /* Cycle Mission automation */
    bool cycle_mission_auto_claim;
    uint32_t cycle_mission_last_action;
    bool cycle_mission_loaded;

    /* Custom Mission automation */
    bool custom_mission_auto_claim;
    uint32_t custom_mission_last_action;
    bool custom_mission_loaded;

    /* Mobilization automation */
    bool mobilization_auto_claim;
    bool mobilization_auto_refresh;
    bool mobilization_auto_buy;
    uint32_t mobilization_last_action;
    bool mobilization_loaded;

    /* Alliance Gather Point automation */
    bool alliance_gather_point_auto_teleport;
    uint32_t alliance_gather_point_last_action;
    bool alliance_gather_point_loaded;

    /* Alliance WhiteList automation */
    bool alliance_whitelist_auto_accept;
    uint32_t alliance_whitelist_last_action;
    bool alliance_whitelist_loaded;

    uint32_t domain_last_log;

    /* Bot activity log: tabular journal of every bot action (gather / build /
     * train / research / claim / heal) written to a local file. Each line has
     * a signed resource delta column plus a per-line resource summary bar.
     * Pure local observability — no gameplay impact, so it defaults ON. */
    bool activity_log;              /* master enable (default true)       */
    char activity_log_path[256];    /* "" => "<data_path>/activity_log.txt"*/
} AutomationSettings;

typedef struct {
    char admin_name[13];
    char command_prefix;
    bool admin_only;
    char data_path[256];
    
    CommandChannel command_input;
    CommandChannel command_output;
} BotSettings;

typedef struct {
	char player_name[13];
	ResourceStock resource;
} PlayerBank;

/*
typedef struct {
	bool enabled;
	bool always_on;
	bool on_attack;
	bool on_scout;
	
	uint16_t priority[8];
	uint8_t priority_count;
} ShieldSettings;
*/

/*
typedef enum
{
	EWATCHTOWER_LINE_TARGET_CAPITAL,
	EWATCHTOWER_LINE_TARGET_CAMP,
	EWATCHTOWER_LINE_TARGET_AMBUSH,
	EWATCHTOWER_ADDLINE_WONDER1,
	EWATCHTOWER_ADDLINE_WONDER2,
	EWATCHTOWER_ADDLINE_WONDER3,
	EWATCHTOWER_ADDLINE_WONDER4,
	EWATCHTOWER_ADDLINE_WONDER5,
	EWATCHTOWER_ADDLINE_WONDER6,
	EWATCHTOWER_ADDLINE_WONDER7
} EWATCHTOWER_LINE_TARGET;

typedef struct {
    uint32_t line_id;
    uint8_t line_type;
    uint64_t begin_time;
    uint32_t require_time;
    EWATCHTOWER_LINE_TARGET target;
} WatchTowerEvent;
*/

typedef struct {
    bool active;
    bool loaded; // Have we received the buff list yet?
    bool pending;
    uint16_t item_id;
    uint16_t quantity;
    uint64_t begin_time;
    uint32_t duration;
} ShieldInfo;

typedef enum {
    HYPER_STATE_IDLE = 0,          // Nothing to do.
    HYPER_STATE_FIND_TARGET,       // Waiting for ally location.
    HYPER_STATE_READY,             // Target found, ready to transfer.
    HYPER_STATE_SENDING,           // Sending one or more marches.
    HYPER_STATE_WAIT_RETURN,       // Waiting for marches to return.
} HyperState;

typedef struct {
	bool enabled;
	char target_name[13];
	uint16_t max_transfer_distance;
	
	HyperState state;
    bool pending;              // Waiting for server response.

	struct {
		uint32_t send_at;
		uint32_t stop_at;
	} food, rock, wood, ore, gold;
	
	uint16_t zone_id;
	uint8_t point_id;
	
} HyperSettings;

typedef struct {
	bool loaded;
	uint32_t total;
	uint32_t infantry[4];
	uint32_t cavalry[4];
	uint32_t ranged[4];
	uint32_t siege[4];
	uint32_t t5_data[4];
} TroopData;

#define GATHER_MAX_TILES 2048
#define GATHER_HERO_SLOTS 5

/*
 * The three gathering venues the engine can drive.  Each is a different
 * target-selection + map-scan driver, but ALL dispatch resource marches through
 * the single shared 6615/2415 troop-march path (see protocol.c).  The venue is
 * bookkeeping only -- it is never encoded into the march frame.
 *
 *   KINGDOM : world map (2201 -> 2220 resource records).
 *   GBG     : Guild Expedition, the Alliance Battlefield (11403/11412 -> 11404,
 *             instance map 2233/2234).
 *   CHAOS   : Chaos Arena, the Solo Battlefield (11992 -> 11993).
 */
typedef enum {
    GATHER_VENUE_KINGDOM = 0,
    GATHER_VENUE_GBG,      /* Guild Expedition (Alliance Battlefield) */
    GATHER_VENUE_CHAOS,    /* Chaos Arena (Solo Battlefield) */
    GATHER_VENUE_COUNT
} GatherVenue;

typedef struct {
    bool active;
    /* verified=1 only after the point passed the dump-attested resource
       record validation (explicit pointKind in 1..6 + full ResourcesPoint
       payload gates).  A tile without verified must never be marched. */
    bool verified;
    uint16_t zone_id;
    uint8_t point_id;
    uint8_t point_kind;
    uint8_t level;
    uint32_t count;
    float rate;
    uint64_t time;
    uint16_t kingdom_id;
} GatherTile;

typedef struct {
    bool enabled;
    /* Per-venue master switches, independent of the kingdom `enabled`.
       Each venue drives its own map-scan but shares the single march slot. */
    bool gbg_enabled;
    bool chaos_enabled;
    /* Which venue dispatched the (single) in-flight march; used to route the
       2416 ack/backoff so one venue's static-stub retry does not silence the
       others. */
    GatherVenue pending_venue;
    /* Whether the venue's enter/battlefield-info handshake is complete. */
    bool venue_entered[GATHER_VENUE_COUNT];
    /* Earliest wall-clock time this venue may send its next map-data/battle-
       field request (per-venue request throttle; kingdom keeps `last_map_request`). */
    uint32_t venue_last_req[GATHER_VENUE_COUNT];
    /* Per-venue backoff window on static-stub rejection (see gathering.c). */
    uint32_t venue_suspend_until[GATHER_VENUE_COUNT];
    bool spare_army;
    bool highest_level_first;
    bool low_tier_first;
    bool lowest_resource;
    bool clearable_only;
    bool gems_ignore_level;
    bool recall_camps;
    bool gathering_gear;
    bool resource_enabled[7];  // 1=food, 2=stone, 3=ore, 4=wood, 5=gold, 6=gems
    bool level_enabled[6];     // 1-5
    uint8_t max_armies;
    uint16_t delay_seconds;
    uint32_t max_travel_seconds;
    uint8_t search_multiplier;
    /*
     * Gather-march destination reach (world units).  A real 6615 becomes a
     * genuine GatherMarching (type 7) only when the destination is a real
     * resource node OUTSIDE the city's own protected ring; a destination on
     * the city's own near tiles comes back Standby (type 0).  Tiles closer
     * than this to the player's city are rejected by find_tile().
     */
    uint32_t min_gather_distance;
    /* Southward floor, world units: the proven gather capture the server
       answered type=7 (real create) came back Y=569, ~57 units below the
       castle (Y~501) and clear of the ~64-unit protected ring.  The euclidean
       min_gather_distance gate alone lets at-latitude EAST tiles pass (dist
       ~= dx), which the server still answers Standby/update.  This floor
       rejects any node whose Y is not at least this far south of the city,
       forcing dispatch onto the same protected-ring-free geometry as the
       working capture.  0 disables the floor. */
    uint32_t min_south_units;
    /* EXPERIMENT: send the kingdom gather via 2415 (_MSG_REQUEST_TROOPMARCH,
       the full MarchEventData serializer) instead of 6615 (the GBG/Chaos
       sparse NOTATK format).  6615 is currently hardcoded for every venue;
       kingdom may want 2415.  Default false. */
    bool send_2415;
    /* EXPERIMENT: send kingdom gather as the OLD FAT 6615 body (167-byte
       hero[5]+troop[16]+pt+pet[5]+troop[16]+T5[4], frame=175) instead of the
       full 141-byte MarchEventDataType body.  Only consulted when send_2415 is
       false.  Default false => full 141-byte body over 6615 (reference bot). */
    bool send_6615_fat;
    /* EXPERIMENT: send the sparse 111-byte captured body over 6615 instead of
       the full 141-byte MarchEventDataType body.  Only consulted when send_2415
       is false.  Default false => full 141-byte body over 6615 (reference bot). */
    bool send_6615_sparse;
    /* EXPERIMENT: optional pins for the sparse 6615 request.  The 6615 body's
       only identity fields are body[46:48] (0x51cc gather node-id from the real
       capture) and body[78:80] (destination map Y).  When >0 these OVERRIDE the
       live target so we can replay the captured node / sweep node derivations
       without recompiling.  0 = leave the live-tile behavior unchanged. */
    uint32_t pin_node_id;   /* write these low-16 as body[46:48] if nonzero */
    bool pin_node_id_zero;  /* EXPERIMENT: write body[46:48]=0 (ignore 0x51cc default) */
    uint32_t pin_dest_y;    /* write low-16 as body[78:80] if nonzero */
    /* EXPERIMENT (world-coordinate model): the captured real gather node
       0x51cc (20939) equals the selected south tile's HOME-scale mp.x (377)
       plus a constant kingdom/world base of 20562.  When nonzero, add this
       to the live tile's mp.x before writing body[46:48], so a real scanned
       south resource tile is sent at its true world X. 0 = disabled (unchanged
       home-scale behavior).  This is the Kingdom-Battle / GBG venue world base,
       NOT a normal-kingdom offset (normal kingdom gather stays home-scale). */
    uint32_t world_x_offset;
    /* Mirror of world_x_offset for the destination tile's map Y (body[78:80]).
       The world model maps home-scale mp.y to world Y by adding a constant
       base (67 for this kingdom, per the byte-verified capture).  Applied only
       when nonzero, by itself and independently of world_x_offset, so a venue
       can enable Y, X, or both. 0 = disabled (unchanged home-scale Y).  The
       old comment claiming "Y unchanged" was wrong for the world model; the
       normal-kingdom default (both offsets 0) does leave Y unchanged. */
    uint32_t world_y_offset;
    /* How many raw 16-unit zones outward (in both X and Y) the map sweep
       requests past the city's own zone, so far real nodes enter the cache. */
    uint8_t scan_reach;
    uint32_t minimum_tile_count;
    uint16_t hero_ids[GATHER_HERO_SLOTS];
    uint32_t schedule_start_min;  // minutes from midnight
    uint32_t schedule_end_min;    // minutes from midnight
    bool schedule_enabled;
    uint32_t last_map_request;
    uint32_t last_send;
    uint32_t pending_until;
    uint8_t local_outstanding_marches;
    uint8_t pending_base_marches;
    uint32_t last_march_ack_result;
    bool march_ack_seen;
    uint16_t pending_zone_id;
    uint8_t pending_point_id;
    /* Consecutive 6615 acknowledgements that were never dispatched as a
       gather march (server classified them Standby and current_marches
       stayed 0).  After GATHER_STANDBY_REJECT_LIMIT in a row, gathering
       suspends for GATHER_STANDBY_SUSPEND_SECONDS so a mis-framed request
       cannot hammer the server forever. */
    uint8_t standby_rejects;
    uint32_t suspend_until;
    uint32_t recent_tile_until;
    uint16_t recent_tile_zone_id;
    uint8_t recent_tile_point_id;
    uint16_t tile_count;
    uint16_t scan_cursor;
    uint16_t last_scan_base_zone;
    GatherTile tiles[GATHER_MAX_TILES];
} GatheringState;

typedef struct {
	bool loaded;
	TroopData troop;
	TroopData healing;
	long num;
	uint total_time;
} WoundedTroopData;



typedef struct {
    bool pending;
    int64_t execute_time;
    char leader[13];
    uint8_t level;
} PendingRally;

/* Daily Mission / Battle Pass state */
typedef struct {
    bool loaded;
    uint32_t activity_group_id;
    uint64_t total_points;
    uint64_t claimed_points;
    uint8_t reward_stages_count;
    uint16_t reward_stage_ids[16];
    bool reward_stage_claimed[16];
    uint32_t missions_count;
    struct {
        uint16_t mission_id;
        uint8_t mission_kind;
        uint64_t current_progress;
        uint64_t target_progress;
        bool completed;
        bool claimed;
    } missions[32];
} DailyMissionState;

/* VIP Mission state */
typedef struct {
    bool loaded;
    uint8_t vip_level;
    uint64_t vip_points;
    uint32_t missions_count;
    struct {
        uint16_t mission_id;
        uint8_t mission_kind;
        uint64_t current_progress;
        uint64_t target_progress;
        bool completed;
        bool claimed;
    } missions[16];
} VipMissionState;

/* Daily Sign-in state */
typedef struct {
    bool loaded;
    uint8_t current_day;
    uint8_t total_days;
    bool signed_today;
    bool can_choose_hero;
    uint16_t hero_choices[5];
    uint8_t hero_choices_count;
    uint64_t next_reset_time;
} DailySigninState;

/* Pet Training state */
typedef struct {
    bool loaded;
    uint8_t training_slots_count;
    struct {
        bool active;
        uint8_t slot_index;
        uint16_t pet_id;
        uint8_t training_type;
        uint64_t start_time;
        uint32_t duration;
        uint8_t pet_level;
        uint8_t pet_star;
    } slots[5];
} PetTrainingState;

/* Hero System state */
typedef struct {
    bool loaded;
    uint16_t heroes_count;
    struct {
        uint16_t hero_id;
        uint8_t level;
        uint8_t star;
        uint8_t quality;
        uint32_t exp;
        uint32_t skill_levels[4];
        bool enhancement_active;
        uint64_t enhancement_finish_time;
    } heroes[32];
} HeroState;

/* Item Crafting state */
typedef struct {
    bool loaded;
    uint16_t recipes_count;
    struct {
        uint16_t recipe_id;
        uint8_t craft_type;
        uint16_t result_item_id;
        uint16_t result_count;
        struct {
            uint16_t item_id;
            uint32_t count;
        } materials[8];
        uint8_t materials_count;
        bool crafting_active;
        uint64_t finish_time;
    } recipes[32];
} ItemCraftState;

/* Achievement state */
typedef struct {
    bool loaded;
    uint32_t activities_count;
    struct {
        uint16_t activity_id;
        uint16_t activity_group_id;
        uint8_t achievement_kind;
        uint64_t current_points;
        uint64_t target_points;
        uint16_t reward_ids[8];
        bool reward_claimed[8];
        uint8_t rewards_count;
    } activities[16];
} AchievementState;

/* Quest Chapter state */
typedef struct {
    bool loaded;
    uint16_t current_chapter;
    uint16_t completed_chapters;
    uint32_t quests_count;
    struct {
        uint16_t quest_id;
        uint8_t quest_type;
        uint64_t progress;
        uint64_t target;
        bool completed;
        bool claimed;
    } quests[64];
} QuestChapterState;

/* Expedition state */
typedef struct {
    bool loaded;
    bool info_loaded;
    uint16_t expedition_id;
    uint8_t stage;
    uint8_t max_stage;
    uint64_t progress;
    uint64_t target;
    bool unlocked;
    bool prize_claimed;
    uint64_t next_refresh_time;
    uint32_t last_update;
} ExpeditionState;

/* Valhalla state */
typedef struct {
    bool loaded;
    bool info_loaded;
    bool prize_loaded;
    bool prize_available;
    uint16_t valhalla_level;
    uint64_t resources[4]; // food, wood, stone, ore
    uint32_t troop_count;
    bool instant_revive_available;
    bool divine_revive_available;
    uint64_t next_refresh_time;
    uint32_t last_update;
} ValhallaState;

/* Adventure state */
typedef struct {
    bool loaded;
    bool info_loaded;
    uint16_t mission_id;
    uint8_t mission_type;
    uint64_t progress;
    uint64_t target;
    bool completed;
    bool prize_claimed;
    uint16_t hunt_monsters_count;
    struct {
        uint16_t monster_id;
        uint8_t level;
        uint16_t zone_id;
        uint8_t point_id;
    } monsters[16];
    uint64_t next_refresh_time;
    uint32_t last_update;
} AdventureState;

/* Relics state */
typedef struct {
    bool loaded;
    bool info_loaded;
    uint16_t gacha_list_count;
    struct {
        uint16_t gacha_id;
        uint8_t gacha_type;
        uint16_t price;
        uint16_t currency_type;
    } gacha_list[16];
    uint16_t relics_count;
    struct {
        uint16_t relic_id;
        uint8_t level;
        uint8_t quality;
        uint64_t exp;
    } relics[32];
    uint64_t coin_points;
    uint64_t next_refresh_time;
    uint32_t last_update;
} RelicsState;

/* Serial Gift state */
typedef struct {
    bool loaded;
    bool list_loaded;
    uint16_t event_count;
    uint16_t gifts_count;
    struct {
        uint16_t event_id;
        uint16_t gift_id;
        uint8_t status; // 0=unclaimed, 1=claimed, 2=expired
        uint64_t expire_time;
        bool available;
        bool claimed;
    } gifts[32];
    uint64_t next_refresh_time;
    uint32_t last_update;
} SerialGiftState;

/*
 * Old Player Back state.
 * Claim-only subsystem reconstructed from the decompiled client stub
 * (Recv_OLDPLAYERBACK_INFO / Send_OLDPLAYERBACK_GETGIFT). The dump bodies are
 * empty, so field layouts are BEST-EFFORT and gated behind AUTO=OFF. No
 * purchase / welcome-flag / free-crossover paths are touched.
 */
typedef struct {
    bool loaded;
    bool event_loaded;   /* challenge window active */
    bool gift_available; /* a veteran-return gift can be claimed */
    bool gift_claimed;
    uint32_t last_update;
} OldPlayerBackState;

/*
 * Gift Activity state.
 * Claim-only subsystem reconstructed from the decompiled client stub
 * (MsgResp_GiftActivity_List / Open_Gift_Box). Field layouts are BEST-EFFORT.
 * Only the free open-gift-box path is used; no buy / random-unlock / spend.
 */
typedef struct {
    bool loaded;
    uint16_t box_count;
    struct {
        uint16_t box_id;
        uint8_t state; /* 0=closed, 1=openable, 2=claimed */
        bool available;
        bool claimed;
    } boxes[32];
    uint32_t last_update;
} GiftActivityState;

/* Week Challenge state */
typedef struct {
    bool loaded;
    bool info_loaded;
    bool prize_loaded;
    bool prize_available;
    uint16_t challenge_id;
    uint8_t week;
    uint64_t score;
    uint16_t rank;
    uint16_t shop_items_count;
    struct {
        uint16_t item_id;
        uint16_t price;
        uint16_t currency_type;
        uint8_t limit;
        uint8_t bought;
    } shop_items[32];
    bool prize_claimed;
    uint64_t next_refresh_time;
    uint32_t last_update;
} WeekChallengeState;

/* Lucky Card state */
typedef struct {
    bool loaded;
    bool info_loaded;
    uint16_t board_id;
    uint8_t unlocked_count;
    uint8_t total_cards;
    uint16_t prizes_count;
    struct {
        uint16_t prize_id;
        uint8_t card_index;
        bool claimed;
    } prizes[16];
    uint64_t next_refresh_time;
    uint32_t last_update;
} LuckyCardState;

/* Dark Nest state */
typedef struct {
    bool loaded;
    bool rally_list_loaded;
    uint16_t rally_count;
    struct {
        uint32_t rally_id;
        uint16_t monster_id;
        uint8_t monster_level;
        uint16_t zone_id;
        uint8_t point_id;
        uint64_t start_time;
        uint64_t end_time;
        uint8_t member_count;
    } rallies[30];
    uint64_t next_refresh_time;
    uint32_t last_update;
} DarkNestState;

/* Fantasy Realm state */
typedef struct {
    bool loaded;
    bool info_loaded;
    uint16_t realm_id;
    uint8_t auto_hunt_enabled;
    uint16_t monsters_count;
    struct {
        uint16_t monster_id;
        uint8_t level;
        uint16_t zone_id;
        uint8_t point_id;
        bool hunted;
    } monsters[32];
    bool summon_available;
    uint64_t next_refresh_time;
    uint32_t last_update;
} FantasyRealmState;

/* Growth Fund state */
typedef struct {
    bool loaded;
    bool info_loaded;
    bool prize_available;
    uint16_t required_level;
    uint16_t fund_level;
    uint64_t total_invested;
    uint16_t rewards_count;
    struct {
        uint16_t level;
        uint16_t reward_id;
        bool claimed;
    } rewards[32];
    uint64_t next_refresh_time;
    uint32_t last_update;
} GrowthFundState;

/* Treasure Back Event state */
typedef struct {
    bool loaded;
    bool info_loaded;
    uint16_t event_id;
    uint64_t points;
    uint16_t prizes_count;
    struct {
        uint16_t prize_id;
        uint8_t tier;
        bool available;
        bool claimed;
        uint64_t required_points;
    } prizes[32];
    uint64_t next_refresh_time;
    uint32_t last_update;
} TreasureBackEventState;

/* Newbie Challenge state */
typedef struct {
    bool loaded;
    bool info_loaded;
    bool prize_available;
    uint16_t next_prize_id;
    bool is_vip;
    uint16_t challenge_id;
    uint64_t score;
    uint16_t vip_prizes_count;
    uint16_t all_prizes_count;
    struct {
        uint16_t prize_id;
        bool claimed;
    } vip_prizes[16];
    struct {
        uint16_t prize_id;
        bool claimed;
    } all_prizes[16];
    uint64_t next_refresh_time;
    uint32_t last_update;
} NewbieChallengeState;

/* Cycle Mission state */
typedef struct {
    bool loaded;
    bool info_loaded;
    bool prize_available;
    uint16_t combo_id;
    uint16_t combo_count;
    struct {
        uint16_t combo_id;
        uint8_t stage;
        uint64_t progress;
        uint64_t target;
        bool completed;
        bool prize_claimed;
    } combos[16];
    uint64_t next_refresh_time;
    uint32_t last_update;
} CycleMissionState;

/* Custom Mission state */
typedef struct {
    bool loaded;
    bool info_loaded;
    uint16_t mission_count;
    struct {
        uint16_t mission_id;
        uint8_t mission_type;
        uint64_t progress;
        uint64_t target;
        bool completed;
        bool prize_claimed;
    } missions[32];
    uint64_t next_refresh_time;
    uint32_t last_update;
} CustomMissionState;

/* Mobilization state */
typedef struct {
    bool loaded;
    bool mission_loaded;
    uint8_t activity_lv;
    uint8_t mission_status;
    uint16_t mission_id;
    uint8_t mission_pos;
    uint8_t mission_kind;
    uint8_t mission_difficulty;
    int64_t mission_time;
    uint32_t mission_target;
    uint32_t complete_score;
    uint32_t am_score;
    uint8_t am_complete_degree;
    uint32_t personal_score;
    uint8_t personal_prize_earned_step;
    uint16_t personal_extra_prize_earned_times;
    uint16_t available_mission;
    uint8_t extra_mission;
    uint8_t involved_member;
    int64_t available_mission_cd_time;
    uint32_t personal_last_stage_point;
    uint8_t next_rank_degree;
    uint8_t more_rewards;
    uint8_t am_gold_state;
    uint64_t ally_mobilization_begin_time;
    uint64_t next_refresh_time;
    uint64_t last_update;
} MobilizationState;

/* Alliance Gather Point state */
typedef struct {
    bool loaded;
    bool point_set;
    uint16_t zone_id;
    uint8_t point_id;
    uint32_t point_changed_time;
    uint64_t next_refresh_time;
    uint64_t last_update;
} AllianceGatherPointState;

/* Alliance WhiteList state */
typedef struct {
    bool loaded;
    uint8_t data_count;
    struct {
        int64_t user_id;
        char name[14];
        char nickname[14];
        int64_t leave_time;
        uint8_t rank;
        bool active;
    } entries[50];
    uint64_t next_refresh_time;
    uint32_t last_update;
} AllianceWhiteListState;


typedef struct {
    // Master switch for all protection features.
    bool enabled;

    /* Shield */

    // Keep a shield active continuously (24/7).
    bool shield_always_on;

    // Use a shield when an incoming attack is detected.
    bool shield_on_incoming_attack;

    // Use a shield when an incoming scout is detected.
    bool shield_on_incoming_scout;

    // Shield item preference.
    uint8_t shield_priority_count;
    uint16_t shield_priority[8];

    /* Troop recall */

    // Recall troops when an incoming attack is detected.
    bool recall_on_incoming_attack;

    // Recall troops when an incoming scout is detected.
    bool recall_on_incoming_scout;
    
    bool recall_on_incoming_conflict;
} ProtectionSettings;

/*
typedef enum {
    T1 = 0,
    T2,
    T3,
    T4,
    T5
} TroopTier;
*/

typedef enum {
    TROOP_INFANTRY = 0,
    TROOP_RANGED   = 1,
    TROOP_CAVALRY  = 2,
    TROOP_SIEGE    = 3
} TroopKind;

typedef enum {
    TIER_T1 = 0,
    TIER_T2 = 1,
    TIER_T3 = 2,
    TIER_T4 = 3,
    TIER_T5 = 4
} TroopTier;

typedef enum {
    DARKNEST_FORMATION_FIXED,
    DARKNEST_FORMATION_LEADER
} DarknestFormationMode;

typedef struct {
    // Enable automatic Darknest rally joining.
    bool enabled;
    bool auto_join;
    
    // Join only Darknest levels within this range.
    uint8_t min_level;
    uint8_t max_level;
    uint8_t max_march;
    
    uint16_t max_distance;
    bool auto_transmute;
    uint8_t essence_level;

    // Total troops to send.
    uint32_t troop_count;
    uint32_t min_join_troops;
    
    // Formation (e.g. 8480 = 80% Infantry, 40% Ranged, 80% Cavalry).
    uint16_t formation;
    // Formation selection mode.
    DarknestFormationMode formation_mode;
    
    uint8_t tier_priority_count;
    // Tier consumption order.
    TroopTier tier_order[5];

    // Random join delay (seconds).
    uint16_t min_join_delay;
    uint16_t max_join_delay;
    
    // Reserve marches for other activities.
    uint8_t max_marches_to_use;

    // Don't join if troop count falls below this percentage.
    uint8_t min_troop_percent;
} DarknestSettings;

typedef struct {
    uint32_t active_rally_count;
    uint32_t being_rally_count;
} RallyState;

typedef struct {
    uint16_t research_tech;
    uint8_t  unk;
    int64_t  finish_time;
    uint32_t total_time;
    uint8_t  tech_data[200];
} TechnologyInfo;


typedef struct {
	int64_t data_index;
	char leader[13];
	uint8_t level;
} DarknestRally;

typedef struct {
	double current;              // Current resource amount.
	uint32_t capacity;           // Maximum storage capacity.

	int64_t production_hour;     // Production per hour (negative = consumption).
	double production_second;    // Production per second.

	double update_timer;         // 1-second update accumulator.
} ResourceTracker;


typedef struct {
    char     name[13];
    uint8_t  vip;
    uint8_t  rank;
    int64_t  begin_time;
    uint32_t require_time;
    uint32_t troop_total;
    uint32_t troops[20];
} RallyMember;

typedef struct {
    uint32_t index;             // Rally index/id.
    uint8_t  kind;              // Rally type.

    int64_t  begin_time;        // Rally start time.
    uint32_t require_time;      // Time until march starts.

    /* Rally leader */
    uint16_t ally_zone_id;
    uint8_t  ally_point_id;
    uint16_t ally_head;
    char     ally_name[13];
    uint8_t  ally_vip;
    uint8_t  ally_rank;

    uint32_t ally_curr_troop;
    uint32_t ally_max_troop;

    uint16_t ally_home_kingdom;

    /* Target (Darknest) */
    uint16_t enemy_head;        // Always UINT16_MAX for NPC rallies.
    uint16_t enemy_zone_id;
    uint8_t  enemy_point_id;
    uint8_t  enemy_vip;
    uint16_t enemy_npc_id;

} NPCRally;

typedef struct {
	uint8_t  type;
	uint32_t index;
	uint8_t  kind;
	
	int64_t  begin_time;
	uint32_t require_time;
	
	/* Rally leader */
	uint16_t ally_zone_id;
	uint8_t  ally_point_id;
	uint16_t ally_head;
	char     ally_name[13];
	uint8_t  ally_vip;
	uint8_t  ally_rank;
	
	uint32_t ally_curr_troop;
	uint32_t ally_max_troop;
	
	/* Rally target */
	uint16_t enemy_zone_id;
	uint8_t  enemy_point_id;
	uint16_t enemy_head;
	char     enemy_name[13];
	uint8_t  enemy_vip;
	uint8_t  enemy_rank;
	
	char     enemy_alliance_tag[4]; // +1 for '\0'
	uint16_t enemy_home_kingdom;
} Rally;


typedef enum {
    TRANSFER_IDLE,
    TRANSFER_FIND_TARGET,
    TRANSFER_WAIT_TARGET,
    TRANSFER_SEND_MARCH,
    TRANSFER_WAIT_MARCH,
    TRANSFER_COMPLETE,
    TRANSFER_FAILED
} TransferState;


typedef struct {
	char issued_name[13]; // Who initiated resource command?
    char target_name[13]; // Who will receive resource?

    ResourceType resource_type;
    ResourceStock resource;
    
    time_t timeout;
    
    uint8_t max_marches;
    uint8_t cur_marches;
    
    uint32_t amount;
    uint32_t remaining;

    uint16_t zone_id;
    uint8_t point_id;

    TransferState state;
} ResourceTransfer;



#define MAX_ALLIANCE_MEMBER 100

typedef struct {
    int64_t user_id;
    uint16_t head;
    char name[14];
    uint8_t rank;
    uint64_t power;
    uint64_t troop_kill_num;
    int64_t logout_time;
    uint8_t white_list_flag;
} AllianceMember;

typedef struct {
    AllianceMember member[MAX_ALLIANCE_MEMBER];
    uint16_t recv_index;
    uint8_t data_finished;
    uint8_t count;
} AllianceMemberList;

typedef struct {
	// network
	int sock;
	PacketStream stream;
	// authentication
	AuthInfo auth;
	// game items
	Item items[MAX_ITEM_COUNT];
	bool items_loaded;
	// protocol state
	ProtocolState protocol;
	// outgoing packet buffer 
	// PacketBuffer reader;
	
	/*
	Stream sin;
	Stream sout;
	*/
	PacketBuffer sin;
	PacketBuffer sout;
	
	uint8_t data[4096];
	uint16_t size;
	uint16_t offset;
	// server state
	uint64_t server_time;
	time_t last_heartbeat;
	// login state
	bool lobby_login;
	// game server 
	ServerInfo game_server;
	ServerInfo gateway_server;
	
	// client configuration
	AppInfo app;
	// resources 
	ResourceStock resources;      // current resources
	ResourceStock bag_resources;  // consumable resource items
	ResourceProduction production; // Per-hour production
	uint64_t resources_last_update;
	
	ChatState chat;
	
	bool resource_loaded;
	// player information 
	PlayerInfo player;
	// game system 
	BlackMarket market;
	BlackMarketBuy buy;
	
	// resources sending
	// ResourceTransfer transfer;
	
	MailInfo mail;
	
	AllianceHelp help;
	
	uint8_t building_count;
	BuildingInfo building[256];
	
	AllianceGiftList alliance_gifts;
	AllianceSettings alliance;
	SmartUseList smart_use;
	
	HelpSpam help_spam;
	
	BankSettings bank;
	
	BotSettings bot;
	AutomationSettings automation;
	
	AllianceInfo RoleAlliance;
	
	PlayerBank player_bank[1000];
	
	// ShieldSettings shield;
	
	ShieldInfo shield_info;
	
	HyperSettings hyper;
	
	TroopData troop;
	GatheringState gathering;
	
	WoundedTroopData wounded;
	
	ProtectionSettings protection;
	
	PendingRally rally;
	
	DarknestSettings darknest;
	
	RallyState rally_status;
	
	// Research information 
	TechnologyInfo technology;
	
	uint32_t supply_capacity;
	
	ResourceTracker tracker;
	
	NPCRally npc_rallies[30]; // maximum 30 npc rallies hold
	
	Rally ally_rallies[30];     // Rallies opened by our alliance.
	Rally enemy_rallies[30];    // Enemy rallies targeting us.
	
	RallyMember rally_members[30];
	
	ResourceTransfer transfer;
	
	AllianceMemberList alliance_member;

	/* New automation subsystem states */
	DailyMissionState daily_mission;
	VipMissionState vip_mission;
	DailySigninState daily_signin;
	PetTrainingState pet_training;
	HeroState hero;
	ItemCraftState item_craft;
	AchievementState achievement;
	QuestChapterState quest_chapter;
	ExpeditionState expedition;
	ValhallaState valhalla;
	AdventureState adventure;
	RelicsState relics;
	SerialGiftState serial_gift;
	OldPlayerBackState oldplayerback;
	GiftActivityState gift_activity;
	WeekChallengeState week_challenge;
	LuckyCardState lucky_card;
	DarkNestState dark_nest;
	FantasyRealmState fantasy_realm;
	GrowthFundState growth_fund;
	TreasureBackEventState treasure_back_event;
	NewbieChallengeState newbie_challenge;
	CycleMissionState cycle_mission;
	CustomMissionState custom_mission;
	MobilizationState mobilization;
	AllianceGatherPointState alliance_gather_point;
	AllianceWhiteListState alliance_whitelist;
} Connection;

/* API */
int  connect_server(const char *ip, unsigned short port);
void disconnect(Connection *conn);
bool send_packet(Connection *conn, bool enc);
int set_nonblocking(Connection *conn);
void reset_connection(Connection *c);

#endif