#ifndef GAMEASSETS_LOADER_H
#define GAMEASSETS_LOADER_H

#include <stdint.h>
#include <stdbool.h>

/* GameAssets container format:
 * - Magic: 0x0001 (2 bytes)
 * - Info word: 0x0000 (2 bytes)
 * - Data: row_count * row_size bytes
 */

typedef struct {
    uint8_t *data;          // Raw table data
    int data_size;          // Total data size in bytes
    int row_count;          // Number of rows
    int row_size;           // Bytes per row
    char table_name[64];    // Table name for logging
} GameAssetTable;

/* Load a GameAssets table from file */
GameAssetTable* LoadGameAssetTable(const char *filename);

/* Get a specific row by index (0-based) */
uint8_t* GetGameAssetRow(GameAssetTable *table, int row_index);

/* Find row by ID (assumes first field is u16 ID at offset 0) */
uint8_t* FindGameAssetByID(GameAssetTable *table, uint16_t id);

/* Free a loaded table */
void FreeGameAssetTable(GameAssetTable *table);

/* Read field helpers */
uint8_t ReadU8(uint8_t *row, int offset);
uint16_t ReadU16(uint8_t *row, int offset);
uint32_t ReadU32(uint8_t *row, int offset);
uint64_t ReadU64(uint8_t *row, int offset);
int8_t ReadI8(uint8_t *row, int offset);
int16_t ReadI16(uint8_t *row, int offset);
int32_t ReadI32(uint8_t *row, int offset);

/* Global GameAssets directory path */
extern char g_GameAssetsPath[512];

/* Set GameAssets directory (defaults to "GameAssets") */
void SetGameAssetsPath(const char *path);

/* Hero table (Heros.txt: 2,966 rows × 136 bytes) */
typedef struct {
    uint16_t id;           // @0
    uint16_t hero_id;      // @2 (actual hero ID: 1602, 1605, etc.)
    // More fields to be validated from method bodies
    // Field offsets will be extracted from HeroManager methods
} HeroRow;

extern GameAssetTable *g_HeroTable;
bool LoadHeroTable(void);
HeroRow* GetHeroByID(uint16_t hero_id);
void FreeHeroTable(void);

/* Monster table (Monster.txt: 300 rows × 57 bytes) */
typedef struct {
    uint16_t id;           // @0
    uint16_t monster_id;   // @2 (232, 233, 235, etc.)
    // More fields from MonsterManager methods
} MonsterRow;

extern GameAssetTable *g_MonsterTable;
bool LoadMonsterTable(void);
MonsterRow* GetMonsterByID(uint16_t monster_id);
void FreeMonsterTable(void);

/* Pet table (Pet.txt: 67 rows × 81 bytes) */
typedef struct {
    uint16_t id;           // @0
    uint16_t pad;          // @2
    uint16_t pet_id;       // @4 (6528, 6541, 1693, etc.)
    // More fields from PetManager methods
} PetRow;

extern GameAssetTable *g_PetTable;
bool LoadPetTable(void);
PetRow* GetPetByID(uint16_t pet_id);
void FreePetTable(void);

/* Quest table (DailyMission.txt: 191 rows × 28 bytes) */
typedef struct {
    uint16_t id;           // @0
    uint16_t mission_id;   // @2
    // Quest requirements, rewards from QuestManager
} QuestRow;

extern GameAssetTable *g_QuestTable;
bool LoadQuestTable(void);
QuestRow* GetQuestByID(uint16_t quest_id);
void FreeQuestTable(void);

/* Achievement table (Achievements.txt: 65 rows × 25 bytes) */
typedef struct {
    uint16_t id;              // @0
    uint16_t achievement_id;  // @2
    // Achievement data from AchievementManager
} AchievementRow;

extern GameAssetTable *g_AchievementTable;
bool LoadAchievementTable(void);
AchievementRow* GetAchievementByID(uint16_t achievement_id);
void FreeAchievementTable(void);

/* Load all priority tables */
bool LoadAllGameAssets(void);

/* Free all loaded tables */
void FreeAllGameAssets(void);

#endif /* GAMEASSETS_LOADER_H */
