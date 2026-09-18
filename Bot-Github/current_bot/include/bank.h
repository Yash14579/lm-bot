#ifndef BANK_H
#define BANK_H

#define MAX_PLAYERS 200

typedef struct {
    char name[64];
    long food;
    long stone;
    long wood;
    long ore;
    long gold;
    long rss_limit;   /* per-player withdrawal cap (0 = unlimited) */
} PlayerBalance;

// core
PlayerBalance* get_player(const char* name);
int  del_player(const char* name);
void reset_bank(void);

// balance
void set_balance(const char* name, const char* type, long amount);
void add_balance(const char* name, const char* type, long amount);
void transfer_balance(const char* from, const char* to, const char* type, long amount);

// output
void get_balance_text(const char* name, char* out);
void get_bank_total(char* out);
int  get_registered_count(void);

#endif