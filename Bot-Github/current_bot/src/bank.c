#include <stdio.h>
#include <string.h>
#include "bank.h"

static PlayerBalance balances[MAX_PLAYERS];
static int balance_count = 0;

// ─────────────────────────────
// Find or create player
// ─────────────────────────────
PlayerBalance* get_player(const char* name) {
    for (int i = 0; i < balance_count; i++) {
        if (strcmp(balances[i].name, name) == 0) {
            return &balances[i];
        }
    }

    if (balance_count >= MAX_PLAYERS) return NULL;

    strcpy(balances[balance_count].name, name);
    balances[balance_count].food = 0;
    balances[balance_count].stone = 0;
    balances[balance_count].wood = 0;
    balances[balance_count].ore = 0;
    balances[balance_count].gold = 0;
    balances[balance_count].rss_limit = 0;

    balance_count++;
    return &balances[balance_count - 1];
}

// ─────────────────────────────
// Delete player
// ─────────────────────────────
int del_player(const char* name) {
    for (int i = 0; i < balance_count; i++) {
        if (strcmp(balances[i].name, name) == 0) {
            // Shift remaining entries down
            memmove(&balances[i], &balances[i + 1], (balance_count - i - 1) * sizeof(PlayerBalance));
            balance_count--;
            return 1;
        }
    }
    return 0;
}

// ─────────────────────────────
// Reset entire bank
// ─────────────────────────────
void reset_bank(void) {
    balance_count = 0;
}

// ─────────────────────────────
// Set balance
// ─────────────────────────────
void set_balance(const char* name, const char* type, long amount) {
    PlayerBalance* p = get_player(name);
    if (!p) return;

    if (strcmp(type, "food") == 0) p->food = amount;
    else if (strcmp(type, "stone") == 0) p->stone = amount;
    else if (strcmp(type, "wood") == 0) p->wood = amount;
    else if (strcmp(type, "ore") == 0) p->ore = amount;
    else if (strcmp(type, "gold") == 0) p->gold = amount;
    else if (strcmp(type, "rsslimit") == 0) p->rss_limit = amount;
}

// ─────────────────────────────
// Add balance (for deposits later)
// ─────────────────────────────
void add_balance(const char* name, const char* type, long amount) {
    PlayerBalance* p = get_player(name);
    if (!p) return;

    if (strcmp(type, "food") == 0) p->food += amount;
    else if (strcmp(type, "stone") == 0) p->stone += amount;
    else if (strcmp(type, "wood") == 0) p->wood += amount;
    else if (strcmp(type, "ore") == 0) p->ore += amount;
    else if (strcmp(type, "gold") == 0) p->gold += amount;
}

// ─────────────────────────────
// Transfer balance
// ─────────────────────────────
void transfer_balance(const char* from, const char* to, const char* type, long amount) {
    PlayerBalance* p1 = get_player(from);
    PlayerBalance* p2 = get_player(to);
    if (!p1 || !p2) return;

    if (strcmp(type, "food") == 0 && p1->food >= amount) {
        p1->food -= amount; p2->food += amount;
    }
    else if (strcmp(type, "stone") == 0 && p1->stone >= amount) {
        p1->stone -= amount; p2->stone += amount;
    }
    else if (strcmp(type, "wood") == 0 && p1->wood >= amount) {
        p1->wood -= amount; p2->wood += amount;
    }
    else if (strcmp(type, "ore") == 0 && p1->ore >= amount) {
        p1->ore -= amount; p2->ore += amount;
    }
    else if (strcmp(type, "gold") == 0 && p1->gold >= amount) {
        p1->gold -= amount; p2->gold += amount;
    }
}

// ─────────────────────────────
// Get player balance text
// ─────────────────────────────
void get_balance_text(const char* name, char* out) {
    PlayerBalance* p = get_player(name);
    if (!p) return;

    sprintf(out,
        "Balance of %s:\nFood: %ld\nStone: %ld\nWood: %ld\nOre: %ld\nGold: %ld\nRSS Limit: %ld",
        name, p->food, p->stone, p->wood, p->ore, p->gold, p->rss_limit);
}

// ─────────────────────────────
// Bank total
// ─────────────────────────────
void get_bank_total(char* out) {
    long f=0,s=0,w=0,o=0,g=0;

    for (int i = 0; i < balance_count; i++) {
        f += balances[i].food;
        s += balances[i].stone;
        w += balances[i].wood;
        o += balances[i].ore;
        g += balances[i].gold;
    }

    sprintf(out,
        "BANK TOTAL:\nFood: %ld\nStone: %ld\nWood: %ld\nOre: %ld\nGold: %ld",
        f,s,w,o,g
    );
}

// ─────────────────────────────
// Get registered player count
// ─────────────────────────────
int get_registered_count(void) {
    return balance_count;
}