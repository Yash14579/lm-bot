#ifndef BOT_SETTINGS_H
#define BOT_SETTINGS_H

#include <stdint.h>
#include <stdbool.h>

/* Complete bot settings structure matching settings.json */

typedef struct {
    char savedProxy[256];
    bool isTaiwan;
    bool resetConnection;
    int reconnectTime;
    int otherLoginTime;
    int workerSpeed;
    int cmdInterval;
    int resetTime;
    int resetTimeBegin;
    bool saveLogToFile;
    bool dontClearLog;
    bool refreshKey;
    int logResetTime;
    bool postToWebhook;
    bool postKeyExpiry;
    bool postStatusChange;
    bool postScheduleChange;
    bool postOtherLogin;
    bool postRSSLimit;
} ConnectionSettings;

typedef struct {
    int troveTime;
    bool useVipPoints;
    bool useExpItems;
    bool autoOpenChests;
    bool autoAttackSkirmish;
    bool autoAttackFireTrial;
    bool recallTroopsForSkirmish;
    bool autoClaimKingdomGifts;
    float skirmishTroopPercent;
    bool useResourceFromBag;
    bool autoTreasureTrove;
    int troveGemReserve;
    bool useStarScrolls;
    int skirmishChapter;
    int assignedWebhook;
    bool saveGuildStats;
    bool saveFestStats;
    bool saveGuildList;
    int hourReset;
    int giftExpMode;
    bool giftUpload;
    bool scheduleBuildSpam;
    int scheduleBuildSpamHours;
    int scheduleBuildSpamAmount;
    int scheduleBuildSpamDelay;
} MiscSettings;

typedef struct {
    bool autoMarkRead;
    bool autoDeleteGameMail;
    bool autoDeleteGuildMail;
    bool autoDeleteSystemMail;
    bool autoDeleteCombatMail;
} MailSettings;

typedef struct {
    bool sendGuildHelp;
    bool requestGuildHelp;
    bool autoGuildGifts;
    bool autoFortunePackets;
    bool joinShowdown;
    bool autoSlowHelpSpeed;
    int guildCheckDelay;
    int helpCheckDelay;
} GuildSettings;

typedef struct {
    bool dailyLoginGift;
    bool autoVIPQuest;
    bool autoTurfQuest;
    bool autoChapterQuest;
    bool autoAdminQuest;
    bool autoGuildQuest;
    bool adventureLog;
    bool autoMysteryBox;
    bool autoBogo;
    bool collectDailyQuests;
    bool sendEmoji;
    bool attackLabQuest;
    bool attackTycoonQuest;
    bool shelterQuest;
    bool openAllGuildQuest;
    bool openAllAdminQuest;
    int questReserve;
} QuestSettings;

typedef struct {
    bool useSpeedUps;
    bool waitForHelp;
    int waitHelpValue;
    bool ignoreHomeKingdomRule;
    bool generalForBuildOnly;
    bool autoBuildingSpeedUp;
    bool autoResearchSpeedUp;
    bool autoTrainingSpeedUp;
    bool autoHealingSpeedUp;
    bool autoMergingSpeedUp;
    bool autoTrapSpeedUp;
    bool autoTrapRepairSpeedUp;
    bool autoLunarGearSpeedUp;
    bool autoGearSpeedUp;
    bool autoWallSpeedUp;
    bool boostingMode;
    int maxSpeedUpExcessMinutes;
} SpeedUpSettings;

typedef struct {
    bool attackLabyrinth;
    bool attackKingdomTycoon;
    bool useHolyStars;
    bool useLuckTokens;
    bool labOnlyFree;
    bool ktOnlyFree;
    int labyrinthMode;
} TurfQuestSettings;

typedef struct {
    bool useFoodBoost;
    bool useStoneBoost;
    bool useWoodBoost;
    bool useOreBoost;
    bool useGoldBoost;
    bool useGatherBoost;
    bool useReducedUpkeep;
    bool useReduceUpkeep25;
} KingdomBoostSettings;

typedef struct {
    bool allowTrading;
    bool tradeFood;
    bool tradeStone;
    bool tradeWood;
    bool tradeOre;
    bool tradeGold;
    bool ignoreFood;
    bool ignoreStone;
    bool ignoreWood;
    bool ignoreOre;
    bool ignoreGold;
    bool ignoreAnima;
    bool ignoreLunite;
    bool ignoreSpeedUp;
    bool exchangeRssItemOnly;
    bool useRssFromBagIfNeeded;
    int exchangeMinQuality;
} CargoShipSettings;

typedef struct {
    bool joinRallies;
    bool craftEssences;
    bool dontFillRally;
    bool noSiege;
    bool noT5;
    bool oneType;
    bool addBuffers;
    int minEssenceLevel;
    int extraSpace;
    int rallyLimit;
    int maxWalkTime;
    int rejoinWaitTime;
    int rallyTroopType;
    int maxRallyTime;
    bool keepEssSlotFree;
    bool checkLab;
    bool levelToAttack[10];
} RallySettings;

typedef struct {
    bool alwaysOpenShield;
    bool openShieldWhenUnderAttack;
    bool openShieldWhenScouted;
    bool openShieldWhenRallied;
    bool biggerSheildsFirst;
    bool alwaysAntiScout;
    bool useLongerAnti;
    bool antiScoutWhenScout;
    bool recallGatherTroopsWhenUnderAttack;
    bool recallGatherTroopsWhenScouted;
    bool recallGatherTroopsOnConflict;
    bool sendTroopsToRegather;
    bool dontShelterSiege;
    int regatherWaitTime;
    int shieldRandomTime;
    int ShelterType;
    int AttackShelterType;
    int shieldRedeployTime;
    int antiRedeployTime;
    bool recallShelterTroopsAfterAttack;
    int preferredShield;
} JsonProtectionSettings;

typedef struct {
    bool gatherResources;
    bool gatherLowestResources;
    bool ignoreLevelForGems;
    bool clearTiles;
    bool targetHigherLevel;
    bool leaveSpareArmy;
    bool highTier;
    int spareArmyAmount;
    bool recallCamps;
    bool useGatherGear;
    bool useGatherSchedule;
    bool ignoreLevel3GF;
    int maxArmysToSend;
    int maxSearchArea;
    int maxWalkTime;
    int tileMinimum;
    int sendingDelay;
    bool levelToGather[6];
    bool typesToGather[6];
} GatherSettings;

typedef struct {
    bool autoHunting;
    bool sendMonstersToChat;
    bool useBoots;
    bool useEnergyItems;
    bool oneKillHunt;
    bool comboPrediction;
    bool allowSaberfang;
    int huntSearchArea;
    int huntSendDelay;
    int maxWalkTime;
    bool avoidConflict;
    bool avoidGuildConflict;
    float energyPercentage;
    int huntMode;
    float stealPercentage;
    int stealCombo;
    int heroType;
    bool huntLevels[5];
    bool monsterTypes[3];
    int selectedHerosMP[5];
    int selectedHerosM[5];
    bool HuntAnyMonster;
} MonsterSettings;

typedef struct {
    bool autoResearch;
    bool useTargetTable;
    bool useTechnolabes;
    int minTechnoMight;
    int researchPriority[18];
    bool researchEnabled[18];
} ResearchSettings;

typedef struct {
    bool autoBuild;
    bool autoUpgrade;
    bool ignoreSpamTarget;
    bool buildByLowestLevel;
    bool autoBuySecondQueue;
    bool secondQueueSpamOnly;
    bool strictPriority;
    int buildPriority;
    int newSpamTarget;
    int maxBuildLevel;
    int BuildingTarget[44];
} BuildSettings;

typedef struct {
    bool autoTalents;
    int pointsLeft;
    int TalentTarget[47];
} TalentSettings;

typedef struct {
    bool autoHireHeros;
    bool autoUpgradeHeros;
    bool autoEnhanceHeros;
    bool useLevelUpItems;
    bool reviveDeadLeader;
    bool useBraveheartItems;
    int heroPriority[20];
} HeroSettings;

typedef struct {
    bool attackArena;
    bool collectGems;
    bool buyExtraAttempts;
    bool attackGuildmates;
    int attemptsToBuy;
    int minWinChance;
    int maxWinChance;
    int arenaHeroType;
    int arenaDefenderType;
    int selectedHeros[5];
} ArenaSettings;

typedef struct {
    bool autoTrainTroops;
    bool autoHealTroops;
    bool autoHealSanctuary;
    bool autoCraftLunar;
    int lunarAmount;
    int barrackTrainingLimit;
    int nowTroopIndex;
    bool rotateTraining;
    int troopData[9][16];
    int troopData_T5[4];
} TroopSettings;

typedef struct {
    bool autoSwitchGear;
    bool needFixHash;
    bool autoUpgradeGear;
    bool autoCraftGear;
    bool useCabinetExpander;
    int idleGearTime;
    int idleGearSet;
} GearSettings;

typedef struct {
    bool enableSchedule;
    bool recallTroops;
    bool checkShield;
    bool checkAnti;
    bool checkShelter;
    bool randomizeSchedule;
    int randMax;
} ScheduleSettings;

typedef struct {
    bool collectChest;
    bool collectFreeChest;
    bool collectWeeklyChallenge;
    bool appraiseArtifacts;
} ArtifactSettings;

/* Main bot settings container */
typedef struct {
    ConnectionSettings connection;
    MiscSettings misc;
    MailSettings mail;
    GuildSettings guild;
    QuestSettings quest;
    SpeedUpSettings speedUp;
    TurfQuestSettings turfQuest;
    KingdomBoostSettings kingdomBoost;
    CargoShipSettings cargoShip;
    RallySettings rally;
    JsonProtectionSettings protection;
    GatherSettings gather;
    MonsterSettings monster;
    ResearchSettings research;
    BuildSettings build;
    TalentSettings talent;
    HeroSettings hero;
    ArenaSettings arena;
    TroopSettings troop;
    GearSettings gear;
    ScheduleSettings schedule;
    ArtifactSettings artifact;
} JsonBotSettings;

/* Global settings instance */
extern JsonBotSettings g_BotSettings;

/* Load settings from JSON file */
bool LoadBotSettings(const char *settings_file);

/* Save current settings to JSON */
bool SaveBotSettings(const char *settings_file);

/* Apply default settings */
void InitDefaultSettings(JsonBotSettings *settings);

/* Setting query helpers */
bool ShouldAutoGuildHelp(void);
bool ShouldAutoQuest(void);
bool ShouldAutoHunt(void);
bool ShouldAutoGather(void);
bool ShouldAutoResearch(void);
bool ShouldAutoBuild(void);
int GetWorkerSpeed(void);
int GetCmdInterval(void);

#endif /* BOT_SETTINGS_H */
