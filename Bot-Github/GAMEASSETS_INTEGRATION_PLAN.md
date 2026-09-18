# GameAssets Integration - START NOW

**Date:** 2026-09-17 13:56 UTC  
**Status:** All tables parsed successfully  
**Goal:** Integrate GameAssets into bot automation over next 2 months

---

## ✅ WHAT WE JUST CONFIRMED

### Successfully Parsed Tables:
- **Heroes**: 2,966 rows × 136 bytes (hero_id at offset 2)
- **Monsters**: 300 rows × 57 bytes (monster_id at offset 2)
- **Pets**: 67 rows × 81 bytes (pet_id at offset 4)
- **Daily Missions**: 191 rows × 28 bytes
- **Achievements**: 65 rows × 25 bytes

### Already Validated & Wired:
- ✅ Item.txt (3,682 items) - in use for bag top-up
- ✅ buildUP.txt (600 buildings) - in use for build cost gates
- ✅ TechLv.txt (3,425 techs) - in use for research cost gates
- ✅ Soldier.txt (37 troops) - in use for tier validation

---

## 🎯 TODAY'S ACTION PLAN (2-3 hours)

### Step 1: Create GameAssets Automation Framework (30 min)

**Create `current_bot/src/gameassets_loader.c`:**

```c
#include "gameassets_loader.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Load any GameAssets table into memory
GameAssetTable* LoadGameAssetTable(const char *filename) {
    FILE *f = fopen(filename, "rb");
    if (!f) return NULL;
    
    // Read header
    uint16_t magic;
    fread(&magic, 2, 1, f);
    if (magic != 0x0001) {
        fclose(f);
        return NULL;
    }
    
    // Skip info word
    fseek(f, 4, SEEK_SET);
    
    // Read all data
    fseek(f, 0, SEEK_END);
    long file_size = ftell(f);
    long data_size = file_size - 4;
    
    fseek(f, 4, SEEK_SET);
    uint8_t *data = malloc(data_size);
    fread(data, 1, data_size, f);
    fclose(f);
    
    GameAssetTable *table = malloc(sizeof(GameAssetTable));
    table->data = data;
    table->data_size = data_size;
    table->row_count = 0;
    table->row_size = 0;
    
    // Detect row size (try divisors)
    for (int rs = 4; rs < 500; rs++) {
        if (data_size % rs == 0) {
            int rc = data_size / rs;
            // Verify: first ID = 1
            uint16_t first_id = *(uint16_t*)(data);
            if (first_id == 1 && rc > 0 && rc < 50000) {
                table->row_size = rs;
                table->row_count = rc;
                break;
            }
        }
    }
    
    return table;
}

// Get a specific row by index
uint8_t* GetGameAssetRow(GameAssetTable *table, int row_index) {
    if (!table || row_index >= table->row_count) return NULL;
    return table->data + (row_index * table->row_size);
}

// Find row by ID (first field is always u16 ID)
uint8_t* FindGameAssetByID(GameAssetTable *table, uint16_t id) {
    for (int i = 0; i < table->row_count; i++) {
        uint8_t *row = GetGameAssetRow(table, i);
        uint16_t row_id = *(uint16_t*)row;
        if (row_id == id) return row;
    }
    return NULL;
}
```

**Create header `current_bot/include/gameassets_loader.h`:**

```c
#ifndef GAMEASSETS_LOADER_H
#define GAMEASSETS_LOADER_H

#include <stdint.h>
#include <stdbool.h>

typedef struct {
    uint8_t *data;
    int data_size;
    int row_count;
    int row_size;
} GameAssetTable;

GameAssetTable* LoadGameAssetTable(const char *filename);
uint8_t* GetGameAssetRow(GameAssetTable *table, int row_index);
uint8_t* FindGameAssetByID(GameAssetTable *table, uint16_t id);
void FreeGameAssetTable(GameAssetTable *table);

#endif
```

### Step 2: Integrate Hero Table (45 min)

**Add to `current_bot/include/game_hero.h`:**

```c
#ifndef GAME_HERO_H
#define GAME_HERO_H

#include <stdint.h>
#include "gameassets_loader.h"

// Hero data structure (from Heros.txt)
// Row size: 136 bytes
typedef struct {
    uint16_t id;           // @0
    uint16_t hero_id;      // @2 (1602, 1605, etc.)
    // More fields to be validated from method bodies
} HeroData;

// Global hero table (loaded at startup)
extern GameAssetTable *g_HeroTable;

// Load heroes at bot startup
bool LoadHeroTable(const char *gameassets_dir);

// Query functions
HeroData* GetHeroByID(uint16_t hero_id);
const char* GetHeroName(uint16_t hero_id);
bool IsHeroUnlocked(Connection *c, uint16_t hero_id);

#endif
```

**Add to `current_bot/src/hero_loader.c`:**

```c
#include "game_hero.h"
#include <stdio.h>
#include <string.h>

GameAssetTable *g_HeroTable = NULL;

bool LoadHeroTable(const char *gameassets_dir) {
    char path[512];
    snprintf(path, sizeof(path), "%s/Heros.txt", gameassets_dir);
    
    g_HeroTable = LoadGameAssetTable(path);
    if (!g_HeroTable) {
        printf("[ERROR] Failed to load Heros.txt\n");
        return false;
    }
    
    printf("[GAMEASSETS] Loaded %d heroes from Heros.txt\n", 
           g_HeroTable->row_count);
    return true;
}

HeroData* GetHeroByID(uint16_t hero_id) {
    if (!g_HeroTable) return NULL;
    
    // Linear search (can optimize later with hash table)
    for (int i = 0; i < g_HeroTable->row_count; i++) {
        uint8_t *row = GetGameAssetRow(g_HeroTable, i);
        uint16_t row_hero_id = *(uint16_t*)(row + 2);  // hero_id at offset 2
        if (row_hero_id == hero_id) {
            return (HeroData*)row;
        }
    }
    return NULL;
}

const char* GetHeroName(uint16_t hero_id) {
    HeroData *hero = GetHeroByID(hero_id);
    if (!hero) return "Unknown Hero";
    
    // Hero names need to be mapped separately
    // For now, return hero_id as string
    static char name[32];
    snprintf(name, sizeof(name), "Hero_%d", hero_id);
    return name;
}
```

### Step 3: Add Monster Table (30 min)

Same pattern as Hero - create:
- `game_monster.h`
- `monster_loader.c`
- Load at startup

### Step 4: Update main.c to load GameAssets (15 min)

```c
#include "game_hero.h"
#include "game_monster.h"
#include "game_pet.h"

int main(int argc, char *argv[]) {
    // ... existing code ...
    
    // Load GameAssets
    const char *gameassets_dir = "GameAssets";
    if (!LoadHeroTable(gameassets_dir)) {
        printf("[ERROR] Failed to load hero table\n");
    }
    if (!LoadMonsterTable(gameassets_dir)) {
        printf("[ERROR] Failed to load monster table\n");
    }
    if (!LoadPetTable(gameassets_dir)) {
        printf("[ERROR] Failed to load pet table\n");
    }
    
    // ... rest of code ...
}
```

---

## 📋 THIS WEEK'S GOALS (Sept 17-23)

### Day 1 (Today): Foundation
- [x] Validate all priority tables (DONE!)
- [ ] Create GameAssets loader framework
- [ ] Integrate Hero table
- [ ] Test loading at bot startup

### Day 2 (Sept 18): Core Tables
- [ ] Integrate Monster table
- [ ] Integrate Pet table  
- [ ] Integrate Quest/Achievement tables
- [ ] Test all tables load correctly

### Day 3 (Sept 19): Equipment & Talent
- [ ] Integrate Equipment tables (Enhance.txt, EquipEnhance.txt)
- [ ] Integrate Talent tables (Talent.txt, AutoTalent.txt)
- [ ] Create lookup functions

### Day 4 (Sept 20): Battle Pass & Activities
- [ ] Integrate BattlePass tables
- [ ] Integrate Activity/Event tables
- [ ] Integrate Adventure tables

### Day 5 (Sept 21): Shop & Economy
- [ ] Integrate shop tables
- [ ] Integrate mall/store tables
- [ ] Integrate price/cost tables

### Day 6 (Sept 22): Validation & Testing
- [ ] Write unit tests for all loaders
- [ ] Verify all tables load correctly
- [ ] Test query functions work
- [ ] Profile memory usage

### Day 7 (Sept 23): Integration & Documentation
- [ ] Wire up tables to automation systems
- [ ] Document all table structures
- [ ] Create field offset reference
- [ ] Week 1 checkpoint review

---

## 🚀 NEXT STEPS AFTER GAMEASSETS (Week 2)

Once GameAssets are loaded, we can:

1. **Use Hero table** → Implement hero levelup/skill automation
2. **Use Monster table** → Implement monster hunting automation  
3. **Use Quest tables** → Implement auto-claim for quests/achievements
4. **Use Equipment tables** → Implement equipment enhancement
5. **Use Talent tables** → Implement auto-talent allocation

Each automation feature becomes trivial once we have the underlying data!

---

## 📝 COMMANDS TO RUN NOW

```bash
cd "C:\Users\sahdev sinh\Downloads\GameTesting\new_bot\current_bot"

# 1. Create gameassets loader files
touch src/gameassets_loader.c
touch include/gameassets_loader.h

# 2. Create hero integration files
touch src/hero_loader.c
touch include/game_hero_extended.h

# 3. Update CMakeLists.txt to include new files
# (Add src/gameassets_loader.c and src/hero_loader.c)

# 4. Build and test
cmake -B build
cmake --build build

# 5. Test loading
./build/client config.cfg
```

---

## 💡 KEY INSIGHTS

### Why GameAssets First?
- **Data-driven automation** - All game logic references these tables
- **No guessing** - Exact costs, requirements, rewards from data files
- **Scalable** - Add new features by just loading more tables
- **Maintainable** - Game updates just need new GameAssets files

### Why This Works:
1. We have complete table structures (validated today!)
2. We have method bodies showing how game reads them
3. We have dump.cs showing field types
4. We can validate with live testing

---

## 🎯 SUCCESS CRITERIA

By end of Week 1, we should have:
- ✅ 20+ GameAssets tables loaded
- ✅ Hero/Monster/Pet/Quest data accessible
- ✅ All automation can query game data
- ✅ Memory-efficient loading (tables stay in RAM)
- ✅ Fast lookups (hash tables where needed)

---

**LET'S START BUILDING!**

Create the gameassets_loader.c file now and we'll have hero data loaded within the hour! 🚀

---

*Created: 2026-09-17 13:56 UTC*  
*Priority: HIGH - Start immediately*  
*Estimated time: 2-3 hours for foundation*
