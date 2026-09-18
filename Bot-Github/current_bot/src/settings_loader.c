#include "bot_settings.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cJSON.h"

/* Global settings instance */
JsonBotSettings g_BotSettings = {0};

void InitDefaultSettings(JsonBotSettings *settings) {
    memset(settings, 0, sizeof(JsonBotSettings));

    /* Connection defaults */
    settings->connection.workerSpeed = 1000;
    settings->connection.cmdInterval = 2000;
    settings->connection.reconnectTime = 5;
    settings->connection.resetConnection = true;
    settings->connection.refreshKey = true;

    /* Guild defaults */
    settings->guild.sendGuildHelp = true;
    settings->guild.requestGuildHelp = true;
    settings->guild.autoGuildGifts = true;
    settings->guild.guildCheckDelay = 30;
    settings->guild.helpCheckDelay = 30;

    /* Quest defaults */
    settings->quest.dailyLoginGift = true;
    settings->quest.autoVIPQuest = true;
    settings->quest.collectDailyQuests = true;
    settings->quest.questReserve = 150;

    /* Gather defaults */
    settings->gather.gatherResources = true;
    settings->gather.maxArmysToSend = 2;
    settings->gather.maxSearchArea = 2;
    settings->gather.maxWalkTime = 10;

    /* Research/Build defaults */
    settings->research.autoResearch = true;
    settings->build.autoBuild = true;
    settings->build.autoUpgrade = true;
}

bool LoadBotSettings(const char *settings_file) {
    FILE *f = fopen(settings_file, "rb");
    if (!f) {
        printf("[SETTINGS] File not found: %s, using defaults\n", settings_file);
        InitDefaultSettings(&g_BotSettings);
        return false;
    }

    /* Read entire file */
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);

    char *json_str = malloc(size + 1);
    fread(json_str, 1, size, f);
    json_str[size] = '\0';
    fclose(f);

    /* Parse JSON */
    cJSON *root = cJSON_Parse(json_str);
    free(json_str);

    if (!root) {
        printf("[SETTINGS] Failed to parse JSON: %s\n", cJSON_GetErrorPtr());
        InitDefaultSettings(&g_BotSettings);
        return false;
    }

    /* Connection Settings */
    cJSON *conn = cJSON_GetObjectItem(root, "connectionSettings");
    if (conn) {
        cJSON *item;
        if ((item = cJSON_GetObjectItem(conn, "savedProxy"))) {
            strncpy(g_BotSettings.connection.savedProxy, item->valuestring, 255);
        }
        if ((item = cJSON_GetObjectItem(conn, "isTaiwan"))) {
            g_BotSettings.connection.isTaiwan = cJSON_IsTrue(item);
        }
        if ((item = cJSON_GetObjectItem(conn, "resetConnection"))) {
            g_BotSettings.connection.resetConnection = cJSON_IsTrue(item);
        }
        if ((item = cJSON_GetObjectItem(conn, "reconnectTime"))) {
            g_BotSettings.connection.reconnectTime = item->valueint;
        }
        if ((item = cJSON_GetObjectItem(conn, "workerSpeed"))) {
            g_BotSettings.connection.workerSpeed = item->valueint;
        }
        if ((item = cJSON_GetObjectItem(conn, "cmdInterval"))) {
            g_BotSettings.connection.cmdInterval = item->valueint;
        }
        if ((item = cJSON_GetObjectItem(conn, "saveLogToFile"))) {
            g_BotSettings.connection.saveLogToFile = cJSON_IsTrue(item);
        }
        if ((item = cJSON_GetObjectItem(conn, "refreshKey"))) {
            g_BotSettings.connection.refreshKey = cJSON_IsTrue(item);
        }
    }

    /* Misc Settings */
    cJSON *misc = cJSON_GetObjectItem(root, "miscSettings");
    if (misc) {
        cJSON *item;
        if ((item = cJSON_GetObjectItem(misc, "troveTime"))) {
            g_BotSettings.misc.troveTime = item->valueint;
        }
        if ((item = cJSON_GetObjectItem(misc, "useVipPoints"))) {
            g_BotSettings.misc.useVipPoints = cJSON_IsTrue(item);
        }
        if ((item = cJSON_GetObjectItem(misc, "useExpItems"))) {
            g_BotSettings.misc.useExpItems = cJSON_IsTrue(item);
        }
        if ((item = cJSON_GetObjectItem(misc, "autoAttackSkirmish"))) {
            g_BotSettings.misc.autoAttackSkirmish = cJSON_IsTrue(item);
        }
        if ((item = cJSON_GetObjectItem(misc, "autoTreasureTrove"))) {
            g_BotSettings.misc.autoTreasureTrove = cJSON_IsTrue(item);
        }
        if ((item = cJSON_GetObjectItem(misc, "troveGemReserve"))) {
            g_BotSettings.misc.troveGemReserve = item->valueint;
        }
        if ((item = cJSON_GetObjectItem(misc, "skirmishTroopPercent"))) {
            g_BotSettings.misc.skirmishTroopPercent = (float)item->valuedouble;
        }
    }

    /* Mail Settings */
    cJSON *mail = cJSON_GetObjectItem(root, "mailSettings");
    if (mail) {
        cJSON *item;
        if ((item = cJSON_GetObjectItem(mail, "autoMarkRead"))) {
            g_BotSettings.mail.autoMarkRead = cJSON_IsTrue(item);
        }
        if ((item = cJSON_GetObjectItem(mail, "autoDeleteGameMail"))) {
            g_BotSettings.mail.autoDeleteGameMail = cJSON_IsTrue(item);
        }
        if ((item = cJSON_GetObjectItem(mail, "autoDeleteGuildMail"))) {
            g_BotSettings.mail.autoDeleteGuildMail = cJSON_IsTrue(item);
        }
        if ((item = cJSON_GetObjectItem(mail, "autoDeleteSystemMail"))) {
            g_BotSettings.mail.autoDeleteSystemMail = cJSON_IsTrue(item);
        }
        if ((item = cJSON_GetObjectItem(mail, "autoDeleteCombatMail"))) {
            g_BotSettings.mail.autoDeleteCombatMail = cJSON_IsTrue(item);
        }
    }

    /* Guild Settings */
    cJSON *guild = cJSON_GetObjectItem(root, "guildSettings");
    if (guild) {
        cJSON *item;
        if ((item = cJSON_GetObjectItem(guild, "sendGuildHelp"))) {
            g_BotSettings.guild.sendGuildHelp = cJSON_IsTrue(item);
        }
        if ((item = cJSON_GetObjectItem(guild, "requestGuildHelp"))) {
            g_BotSettings.guild.requestGuildHelp = cJSON_IsTrue(item);
        }
        if ((item = cJSON_GetObjectItem(guild, "autoGuildGifts"))) {
            g_BotSettings.guild.autoGuildGifts = cJSON_IsTrue(item);
        }
        if ((item = cJSON_GetObjectItem(guild, "autoFortunePackets"))) {
            g_BotSettings.guild.autoFortunePackets = cJSON_IsTrue(item);
        }
        if ((item = cJSON_GetObjectItem(guild, "joinShowdown"))) {
            g_BotSettings.guild.joinShowdown = cJSON_IsTrue(item);
        }
        if ((item = cJSON_GetObjectItem(guild, "guildCheckDelay"))) {
            g_BotSettings.guild.guildCheckDelay = item->valueint;
        }
        if ((item = cJSON_GetObjectItem(guild, "helpCheckDelay"))) {
            g_BotSettings.guild.helpCheckDelay = item->valueint;
        }
    }

    /* Quest Settings */
    cJSON *quest = cJSON_GetObjectItem(root, "questSettings");
    if (quest) {
        cJSON *item;
        if ((item = cJSON_GetObjectItem(quest, "dailyLoginGift"))) {
            g_BotSettings.quest.dailyLoginGift = cJSON_IsTrue(item);
        }
        if ((item = cJSON_GetObjectItem(quest, "autoVIPQuest"))) {
            g_BotSettings.quest.autoVIPQuest = cJSON_IsTrue(item);
        }
        if ((item = cJSON_GetObjectItem(quest, "autoTurfQuest"))) {
            g_BotSettings.quest.autoTurfQuest = cJSON_IsTrue(item);
        }
        if ((item = cJSON_GetObjectItem(quest, "autoChapterQuest"))) {
            g_BotSettings.quest.autoChapterQuest = cJSON_IsTrue(item);
        }
        if ((item = cJSON_GetObjectItem(quest, "autoAdminQuest"))) {
            g_BotSettings.quest.autoAdminQuest = cJSON_IsTrue(item);
        }
        if ((item = cJSON_GetObjectItem(quest, "autoGuildQuest"))) {
            g_BotSettings.quest.autoGuildQuest = cJSON_IsTrue(item);
        }
        if ((item = cJSON_GetObjectItem(quest, "autoMysteryBox"))) {
            g_BotSettings.quest.autoMysteryBox = cJSON_IsTrue(item);
        }
        if ((item = cJSON_GetObjectItem(quest, "collectDailyQuests"))) {
            g_BotSettings.quest.collectDailyQuests = cJSON_IsTrue(item);
        }
        if ((item = cJSON_GetObjectItem(quest, "questReserve"))) {
            g_BotSettings.quest.questReserve = item->valueint;
        }
    }

    /* SpeedUp Settings */
    cJSON *speedup = cJSON_GetObjectItem(root, "speedUpSettings");
    if (speedup) {
        cJSON *item;
        if ((item = cJSON_GetObjectItem(speedup, "useSpeedUps"))) {
            g_BotSettings.speedUp.useSpeedUps = cJSON_IsTrue(item);
        }
        if ((item = cJSON_GetObjectItem(speedup, "waitForHelp"))) {
            g_BotSettings.speedUp.waitForHelp = cJSON_IsTrue(item);
        }
        if ((item = cJSON_GetObjectItem(speedup, "waitHelpValue"))) {
            g_BotSettings.speedUp.waitHelpValue = item->valueint;
        }
        if ((item = cJSON_GetObjectItem(speedup, "autoBuildingSpeedUp"))) {
            g_BotSettings.speedUp.autoBuildingSpeedUp = cJSON_IsTrue(item);
        }
        if ((item = cJSON_GetObjectItem(speedup, "autoResearchSpeedUp"))) {
            g_BotSettings.speedUp.autoResearchSpeedUp = cJSON_IsTrue(item);
        }
        if ((item = cJSON_GetObjectItem(speedup, "autoTrainingSpeedUp"))) {
            g_BotSettings.speedUp.autoTrainingSpeedUp = cJSON_IsTrue(item);
        }
        if ((item = cJSON_GetObjectItem(speedup, "autoHealingSpeedUp"))) {
            g_BotSettings.speedUp.autoHealingSpeedUp = cJSON_IsTrue(item);
        }
    }

    /* Rally Settings */
    cJSON *rally = cJSON_GetObjectItem(root, "rallySettings");
    if (rally) {
        cJSON *item;
        if ((item = cJSON_GetObjectItem(rally, "joinRallies"))) {
            g_BotSettings.rally.joinRallies = cJSON_IsTrue(item);
        }
        if ((item = cJSON_GetObjectItem(rally, "craftEssences"))) {
            g_BotSettings.rally.craftEssences = cJSON_IsTrue(item);
        }
        if ((item = cJSON_GetObjectItem(rally, "rallyLimit"))) {
            g_BotSettings.rally.rallyLimit = item->valueint;
        }
        if ((item = cJSON_GetObjectItem(rally, "maxWalkTime"))) {
            g_BotSettings.rally.maxWalkTime = item->valueint;
        }
        if ((item = cJSON_GetObjectItem(rally, "rallyTroopType"))) {
            g_BotSettings.rally.rallyTroopType = item->valueint;
        }
    }

    /* Protection Settings */
    cJSON *protect = cJSON_GetObjectItem(root, "protectionSettings");
    if (protect) {
        cJSON *item;
        if ((item = cJSON_GetObjectItem(protect, "alwaysOpenShield"))) {
            g_BotSettings.protection.alwaysOpenShield = cJSON_IsTrue(item);
        }
        if ((item = cJSON_GetObjectItem(protect, "openShieldWhenUnderAttack"))) {
            g_BotSettings.protection.openShieldWhenUnderAttack = cJSON_IsTrue(item);
        }
        if ((item = cJSON_GetObjectItem(protect, "openShieldWhenScouted"))) {
            g_BotSettings.protection.openShieldWhenScouted = cJSON_IsTrue(item);
        }
        if ((item = cJSON_GetObjectItem(protect, "recallGatherTroopsWhenUnderAttack"))) {
            g_BotSettings.protection.recallGatherTroopsWhenUnderAttack = cJSON_IsTrue(item);
        }
    }

    /* Gather Settings */
    cJSON *gather = cJSON_GetObjectItem(root, "gatherSettings");
    if (gather) {
        cJSON *item;
        if ((item = cJSON_GetObjectItem(gather, "gatherResources"))) {
            g_BotSettings.gather.gatherResources = cJSON_IsTrue(item);
        }
        if ((item = cJSON_GetObjectItem(gather, "maxArmysToSend"))) {
            g_BotSettings.gather.maxArmysToSend = item->valueint;
        }
        if ((item = cJSON_GetObjectItem(gather, "maxSearchArea"))) {
            g_BotSettings.gather.maxSearchArea = item->valueint;
        }
        if ((item = cJSON_GetObjectItem(gather, "maxWalkTime"))) {
            g_BotSettings.gather.maxWalkTime = item->valueint;
        }
        if ((item = cJSON_GetObjectItem(gather, "tileMinimum"))) {
            g_BotSettings.gather.tileMinimum = item->valueint;
        }
        if ((item = cJSON_GetObjectItem(gather, "useGatherGear"))) {
            g_BotSettings.gather.useGatherGear = cJSON_IsTrue(item);
        }
    }

    /* Monster Settings */
    cJSON *monster = cJSON_GetObjectItem(root, "monsterSettings");
    if (monster) {
        cJSON *item;
        if ((item = cJSON_GetObjectItem(monster, "autoHunting"))) {
            g_BotSettings.monster.autoHunting = cJSON_IsTrue(item);
        }
        if ((item = cJSON_GetObjectItem(monster, "useBoots"))) {
            g_BotSettings.monster.useBoots = cJSON_IsTrue(item);
        }
        if ((item = cJSON_GetObjectItem(monster, "useEnergyItems"))) {
            g_BotSettings.monster.useEnergyItems = cJSON_IsTrue(item);
        }
        if ((item = cJSON_GetObjectItem(monster, "huntSearchArea"))) {
            g_BotSettings.monster.huntSearchArea = item->valueint;
        }
        if ((item = cJSON_GetObjectItem(monster, "maxWalkTime"))) {
            g_BotSettings.monster.maxWalkTime = item->valueint;
        }
        if ((item = cJSON_GetObjectItem(monster, "energyPercentage"))) {
            g_BotSettings.monster.energyPercentage = (float)item->valuedouble;
        }
    }

    /* Research Settings */
    cJSON *research = cJSON_GetObjectItem(root, "researchSettings");
    if (research) {
        cJSON *item;
        if ((item = cJSON_GetObjectItem(research, "autoResearch"))) {
            g_BotSettings.research.autoResearch = cJSON_IsTrue(item);
        }
        if ((item = cJSON_GetObjectItem(research, "useTargetTable"))) {
            g_BotSettings.research.useTargetTable = cJSON_IsTrue(item);
        }
        if ((item = cJSON_GetObjectItem(research, "useTechnolabes"))) {
            g_BotSettings.research.useTechnolabes = cJSON_IsTrue(item);
        }
    }

    /* Build Settings */
    cJSON *build = cJSON_GetObjectItem(root, "buildSettingsNew");
    if (build) {
        cJSON *item;
        if ((item = cJSON_GetObjectItem(build, "autoBuild"))) {
            g_BotSettings.build.autoBuild = cJSON_IsTrue(item);
        }
        if ((item = cJSON_GetObjectItem(build, "autoUpgrade"))) {
            g_BotSettings.build.autoUpgrade = cJSON_IsTrue(item);
        }
        if ((item = cJSON_GetObjectItem(build, "buildByLowestLevel"))) {
            g_BotSettings.build.buildByLowestLevel = cJSON_IsTrue(item);
        }
        if ((item = cJSON_GetObjectItem(build, "maxBuildLevel"))) {
            g_BotSettings.build.maxBuildLevel = item->valueint;
        }
    }

    /* Hero Settings */
    cJSON *hero = cJSON_GetObjectItem(root, "heroSettings");
    if (hero) {
        cJSON *item;
        if ((item = cJSON_GetObjectItem(hero, "autoHireHeros"))) {
            g_BotSettings.hero.autoHireHeros = cJSON_IsTrue(item);
        }
        if ((item = cJSON_GetObjectItem(hero, "autoUpgradeHeros"))) {
            g_BotSettings.hero.autoUpgradeHeros = cJSON_IsTrue(item);
        }
        if ((item = cJSON_GetObjectItem(hero, "autoEnhanceHeros"))) {
            g_BotSettings.hero.autoEnhanceHeros = cJSON_IsTrue(item);
        }
        if ((item = cJSON_GetObjectItem(hero, "useLevelUpItems"))) {
            g_BotSettings.hero.useLevelUpItems = cJSON_IsTrue(item);
        }
    }

    /* Arena Settings */
    cJSON *arena = cJSON_GetObjectItem(root, "arenaSettings");
    if (arena) {
        cJSON *item;
        if ((item = cJSON_GetObjectItem(arena, "attackArena"))) {
            g_BotSettings.arena.attackArena = cJSON_IsTrue(item);
        }
        if ((item = cJSON_GetObjectItem(arena, "collectGems"))) {
            g_BotSettings.arena.collectGems = cJSON_IsTrue(item);
        }
        if ((item = cJSON_GetObjectItem(arena, "minWinChance"))) {
            g_BotSettings.arena.minWinChance = item->valueint;
        }
        if ((item = cJSON_GetObjectItem(arena, "maxWinChance"))) {
            g_BotSettings.arena.maxWinChance = item->valueint;
        }
    }

    /* Troop Settings */
    cJSON *troop = cJSON_GetObjectItem(root, "troopSettings");
    if (troop) {
        cJSON *item;
        if ((item = cJSON_GetObjectItem(troop, "autoTrainTroops"))) {
            g_BotSettings.troop.autoTrainTroops = cJSON_IsTrue(item);
        }
        if ((item = cJSON_GetObjectItem(troop, "autoHealTroops"))) {
            g_BotSettings.troop.autoHealTroops = cJSON_IsTrue(item);
        }
        if ((item = cJSON_GetObjectItem(troop, "rotateTraining"))) {
            g_BotSettings.troop.rotateTraining = cJSON_IsTrue(item);
        }
    }

    /* Gear Settings */
    cJSON *gear = cJSON_GetObjectItem(root, "gearSettings");
    if (gear) {
        cJSON *item;
        if ((item = cJSON_GetObjectItem(gear, "autoSwitchGear"))) {
            g_BotSettings.gear.autoSwitchGear = cJSON_IsTrue(item);
        }
        if ((item = cJSON_GetObjectItem(gear, "autoUpgradeGear"))) {
            g_BotSettings.gear.autoUpgradeGear = cJSON_IsTrue(item);
        }
        if ((item = cJSON_GetObjectItem(gear, "idleGearTime"))) {
            g_BotSettings.gear.idleGearTime = item->valueint;
        }
    }

    /* Artifact Settings */
    cJSON *artifact = cJSON_GetObjectItem(root, "artifactSettings");
    if (artifact) {
        cJSON *item;
        if ((item = cJSON_GetObjectItem(artifact, "collectChest"))) {
            g_BotSettings.artifact.collectChest = cJSON_IsTrue(item);
        }
        if ((item = cJSON_GetObjectItem(artifact, "collectFreeChest"))) {
            g_BotSettings.artifact.collectFreeChest = cJSON_IsTrue(item);
        }
        if ((item = cJSON_GetObjectItem(artifact, "appraiseArtifacts"))) {
            g_BotSettings.artifact.appraiseArtifacts = cJSON_IsTrue(item);
        }
    }

    cJSON_Delete(root);

    printf("[SETTINGS] Loaded successfully from %s\n", settings_file);
    printf("[SETTINGS] Worker speed: %dms, Cmd interval: %dms\n",
           g_BotSettings.connection.workerSpeed,
           g_BotSettings.connection.cmdInterval);
    printf("[SETTINGS] Guild help: %s, Auto quests: %s\n",
           g_BotSettings.guild.sendGuildHelp ? "ON" : "OFF",
           g_BotSettings.quest.collectDailyQuests ? "ON" : "OFF");
    printf("[SETTINGS] Auto hunt: %s, Auto gather: %s\n",
           g_BotSettings.monster.autoHunting ? "ON" : "OFF",
           g_BotSettings.gather.gatherResources ? "ON" : "OFF");
    printf("[SETTINGS] Auto research: %s, Auto build: %s\n",
           g_BotSettings.research.autoResearch ? "ON" : "OFF",
           g_BotSettings.build.autoBuild ? "ON" : "OFF");

    return true;
}

/* Helper query functions */
bool ShouldAutoGuildHelp(void) {
    return g_BotSettings.guild.sendGuildHelp;
}

bool ShouldAutoQuest(void) {
    return g_BotSettings.quest.collectDailyQuests;
}

bool ShouldAutoHunt(void) {
    return g_BotSettings.monster.autoHunting;
}

bool ShouldAutoGather(void) {
    return g_BotSettings.gather.gatherResources;
}

bool ShouldAutoResearch(void) {
    return g_BotSettings.research.autoResearch;
}

bool ShouldAutoBuild(void) {
    return g_BotSettings.build.autoBuild;
}

int GetWorkerSpeed(void) {
    return g_BotSettings.connection.workerSpeed;
}

int GetCmdInterval(void) {
    return g_BotSettings.connection.cmdInterval;
}
