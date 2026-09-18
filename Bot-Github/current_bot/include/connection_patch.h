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