#include "gameassets_loader.h"
#include "log.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Global GameAssets directory path */
char g_GameAssetsPath[512] = "GameAssets";

/* Global tables */
GameAssetTable *g_HeroTable = NULL;
GameAssetTable *g_MonsterTable = NULL;
GameAssetTable *g_PetTable = NULL;
GameAssetTable *g_QuestTable = NULL;
GameAssetTable *g_AchievementTable = NULL;

void SetGameAssetsPath(const char *path) {
    strncpy(g_GameAssetsPath, path, sizeof(g_GameAssetsPath) - 1);
    g_GameAssetsPath[sizeof(g_GameAssetsPath) - 1] = '\0';
}

GameAssetTable* LoadGameAssetTable(const char *filename) {
    FILE *f = fopen(filename, "rb");
    if (!f) {
        LOGE("[GAMEASSETS] Failed to open: %s\n", filename);
        return NULL;
    }

    /* Read header */
    uint16_t magic;
    if (fread(&magic, 2, 1, f) != 1) {
        fclose(f);
        return NULL;
    }

    if (magic != 0x0001) {
        LOGE("[GAMEASSETS] Invalid magic in %s: 0x%04X\n", filename, magic);
        fclose(f);
        return NULL;
    }

    /* Skip info word, read all data */
    fseek(f, 4, SEEK_SET);
    fseek(f, 0, SEEK_END);
    long file_size = ftell(f);
    long data_size = file_size - 4;

    fseek(f, 4, SEEK_SET);
    uint8_t *data = (uint8_t*)malloc(data_size);
    if (!data) {
        fclose(f);
        return NULL;
    }

    if (fread(data, 1, data_size, f) != (size_t)data_size) {
        free(data);
        fclose(f);
        return NULL;
    }
    fclose(f);

    /* Allocate table structure */
    GameAssetTable *table = (GameAssetTable*)malloc(sizeof(GameAssetTable));
    if (!table) {
        free(data);
        return NULL;
    }

    table->data = data;
    table->data_size = (int)data_size;
    table->row_count = 0;
    table->row_size = 0;

    /* Extract filename for logging */
    const char *basename = strrchr(filename, '/');
    if (!basename) basename = strrchr(filename, '\\');
    if (!basename) basename = filename;
    else basename++;
    strncpy(table->table_name, basename, sizeof(table->table_name) - 1);
    table->table_name[sizeof(table->table_name) - 1] = '\0';

    /* Detect row size by finding divisors where first ID = 1 */
    for (int rs = 4; rs < 500; rs++) {
        if (data_size % rs == 0) {
            int rc = data_size / rs;
            if (rc > 0 && rc < 50000) {
                /* Check if first field looks like ID = 1 */
                uint16_t first_id = *(uint16_t*)(data);
                if (first_id == 1) {
                    table->row_size = rs;
                    table->row_count = rc;
                    break;
                }
            }
        }
    }

    if (table->row_size == 0) {
        LOGE("[GAMEASSETS] Failed to detect row size for %s\n", filename);
        free(data);
        free(table);
        return NULL;
    }

    LOGI("[GAMEASSETS] Loaded %s: %d rows × %d bytes\n",
         table->table_name, table->row_count, table->row_size);

    return table;
}

uint8_t* GetGameAssetRow(GameAssetTable *table, int row_index) {
    if (!table || row_index < 0 || row_index >= table->row_count) {
        return NULL;
    }
    return table->data + (row_index * table->row_size);
}

uint8_t* FindGameAssetByID(GameAssetTable *table, uint16_t id) {
    if (!table) return NULL;

    /* Linear search - can optimize with hash table later */
    for (int i = 0; i < table->row_count; i++) {
        uint8_t *row = GetGameAssetRow(table, i);
        uint16_t row_id = *(uint16_t*)row;
        if (row_id == id) return row;
    }
    return NULL;
}

void FreeGameAssetTable(GameAssetTable *table) {
    if (table) {
        if (table->data) free(table->data);
        free(table);
    }
}

/* Field readers */
uint8_t ReadU8(uint8_t *row, int offset) {
    return row[offset];
}

uint16_t ReadU16(uint8_t *row, int offset) {
    return *(uint16_t*)(row + offset);
}

uint32_t ReadU32(uint8_t *row, int offset) {
    return *(uint32_t*)(row + offset);
}

uint64_t ReadU64(uint8_t *row, int offset) {
    return *(uint64_t*)(row + offset);
}

int8_t ReadI8(uint8_t *row, int offset) {
    return *(int8_t*)(row + offset);
}

int16_t ReadI16(uint8_t *row, int offset) {
    return *(int16_t*)(row + offset);
}

int32_t ReadI32(uint8_t *row, int offset) {
    return *(int32_t*)(row + offset);
}

/* Hero table */
bool LoadHeroTable(void) {
    char path[768];
    snprintf(path, sizeof(path), "%s/Heros.txt", g_GameAssetsPath);

    g_HeroTable = LoadGameAssetTable(path);
    if (!g_HeroTable) {
        LOGE("[GAMEASSETS] Failed to load Heros.txt\n");
        return false;
    }

    LOGI("[GAMEASSETS] Heroes loaded: %d heroes available\n", g_HeroTable->row_count);
    return true;
}

HeroRow* GetHeroByID(uint16_t hero_id) {
    if (!g_HeroTable) return NULL;

    for (int i = 0; i < g_HeroTable->row_count; i++) {
        uint8_t *row = GetGameAssetRow(g_HeroTable, i);
        uint16_t row_hero_id = ReadU16(row, 2);  // hero_id at offset 2
        if (row_hero_id == hero_id) {
            return (HeroRow*)row;
        }
    }
    return NULL;
}

void FreeHeroTable(void) {
    if (g_HeroTable) {
        FreeGameAssetTable(g_HeroTable);
        g_HeroTable = NULL;
    }
}

/* Monster table */
bool LoadMonsterTable(void) {
    char path[768];
    snprintf(path, sizeof(path), "%s/Monster.txt", g_GameAssetsPath);

    g_MonsterTable = LoadGameAssetTable(path);
    if (!g_MonsterTable) {
        LOGE("[GAMEASSETS] Failed to load Monster.txt\n");
        return false;
    }

    LOGI("[GAMEASSETS] Monsters loaded: %d monsters available\n", g_MonsterTable->row_count);
    return true;
}

MonsterRow* GetMonsterByID(uint16_t monster_id) {
    if (!g_MonsterTable) return NULL;

    for (int i = 0; i < g_MonsterTable->row_count; i++) {
        uint8_t *row = GetGameAssetRow(g_MonsterTable, i);
        uint16_t row_monster_id = ReadU16(row, 2);  // monster_id at offset 2
        if (row_monster_id == monster_id) {
            return (MonsterRow*)row;
        }
    }
    return NULL;
}

void FreeMonsterTable(void) {
    if (g_MonsterTable) {
        FreeGameAssetTable(g_MonsterTable);
        g_MonsterTable = NULL;
    }
}

/* Pet table */
bool LoadPetTable(void) {
    char path[768];
    snprintf(path, sizeof(path), "%s/Pet.txt", g_GameAssetsPath);

    g_PetTable = LoadGameAssetTable(path);
    if (!g_PetTable) {
        LOGE("[GAMEASSETS] Failed to load Pet.txt\n");
        return false;
    }

    LOGI("[GAMEASSETS] Pets loaded: %d pets available\n", g_PetTable->row_count);
    return true;
}

PetRow* GetPetByID(uint16_t pet_id) {
    if (!g_PetTable) return NULL;

    for (int i = 0; i < g_PetTable->row_count; i++) {
        uint8_t *row = GetGameAssetRow(g_PetTable, i);
        uint16_t row_pet_id = ReadU16(row, 4);  // pet_id at offset 4
        if (row_pet_id == pet_id) {
            return (PetRow*)row;
        }
    }
    return NULL;
}

void FreePetTable(void) {
    if (g_PetTable) {
        FreeGameAssetTable(g_PetTable);
        g_PetTable = NULL;
    }
}

/* Quest table */
bool LoadQuestTable(void) {
    char path[768];
    snprintf(path, sizeof(path), "%s/DailyMission.txt", g_GameAssetsPath);

    g_QuestTable = LoadGameAssetTable(path);
    if (!g_QuestTable) {
        LOGE("[GAMEASSETS] Failed to load DailyMission.txt\n");
        return false;
    }

    LOGI("[GAMEASSETS] Quests loaded: %d daily missions available\n", g_QuestTable->row_count);
    return true;
}

QuestRow* GetQuestByID(uint16_t quest_id) {
    if (!g_QuestTable) return NULL;

    for (int i = 0; i < g_QuestTable->row_count; i++) {
        uint8_t *row = GetGameAssetRow(g_QuestTable, i);
        uint16_t row_quest_id = ReadU16(row, 2);  // mission_id at offset 2
        if (row_quest_id == quest_id) {
            return (QuestRow*)row;
        }
    }
    return NULL;
}

void FreeQuestTable(void) {
    if (g_QuestTable) {
        FreeGameAssetTable(g_QuestTable);
        g_QuestTable = NULL;
    }
}

/* Achievement table */
bool LoadAchievementTable(void) {
    char path[768];
    snprintf(path, sizeof(path), "%s/Achievements.txt", g_GameAssetsPath);

    g_AchievementTable = LoadGameAssetTable(path);
    if (!g_AchievementTable) {
        LOGE("[GAMEASSETS] Failed to load Achievements.txt\n");
        return false;
    }

    LOGI("[GAMEASSETS] Achievements loaded: %d achievements available\n", g_AchievementTable->row_count);
    return true;
}

AchievementRow* GetAchievementByID(uint16_t achievement_id) {
    if (!g_AchievementTable) return NULL;

    for (int i = 0; i < g_AchievementTable->row_count; i++) {
        uint8_t *row = GetGameAssetRow(g_AchievementTable, i);
        uint16_t row_achievement_id = ReadU16(row, 2);  // achievement_id at offset 2
        if (row_achievement_id == achievement_id) {
            return (AchievementRow*)row;
        }
    }
    return NULL;
}

void FreeAchievementTable(void) {
    if (g_AchievementTable) {
        FreeGameAssetTable(g_AchievementTable);
        g_AchievementTable = NULL;
    }
}

/* Load all priority tables */
bool LoadAllGameAssets(void) {
    LOGI("[GAMEASSETS] Loading GameAssets from: %s\n", g_GameAssetsPath);

    bool success = true;

    if (!LoadHeroTable()) success = false;
    if (!LoadMonsterTable()) success = false;
    if (!LoadPetTable()) success = false;
    if (!LoadQuestTable()) success = false;
    if (!LoadAchievementTable()) success = false;

    if (success) {
        LOGI("[GAMEASSETS] All priority tables loaded successfully!\n");
    } else {
        LOGE("[GAMEASSETS] Some tables failed to load\n");
    }

    return success;
}

/* Free all loaded tables */
void FreeAllGameAssets(void) {
    FreeHeroTable();
    FreeMonsterTable();
    FreePetTable();
    FreeQuestTable();
    FreeAchievementTable();
    LOGI("[GAMEASSETS] All tables freed\n");
}
