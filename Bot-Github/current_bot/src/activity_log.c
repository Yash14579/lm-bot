#include "activity_log.h"
#include "log.h"

#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>

/*
 * Activity log implementation. State is a file-scope singleton (the log has no
 * gameplay state and touches no connection struct beyond what the callers
 * already carry), so Connection only needs the two config fields in
 * AutomationSettings. Every writer is bounds-checked and text-only; the module
 * never sends a packet and never allocates on the push path.
 */

static FILE *s_fp = NULL;
static char s_path[300];
static uint32_t s_seq = 0;
static bool  s_have_prev = false;
static ResourceStock s_prev;

/* Resource column order mirrors the in-game bar: F=Food, S=Stone, W=Wood,
 * O=Ore, G=Gold. */
enum { RI_FOOD, RI_ROCK, RI_WOOD, RI_ORE, RI_GOLD, RI_COUNT };
static const char *const kResLetter[RI_COUNT] = { "F", "S", "W", "O", "G" };
static const char *const kResName[RI_COUNT] =
    { "Food", "Stone", "Timber", "Ore", "Gold" };

static int64_t res_at(const ResourceStock *r, int i)
{
    switch (i) {
        case RI_FOOD: return r->food;
        case RI_ROCK: return r->rock;
        case RI_WOOD: return r->wood;
        case RI_ORE:  return r->ore;
        default:      return r->gold;
    }
}

static void res_set(ResourceStock *dst, const ResourceStock *src)
{
    dst->food = src->food; dst->rock = src->rock; dst->wood = src->wood;
    dst->ore  = src->ore;  dst->gold = src->gold;
}

const char *ActivityLogKindName(ActivityKind k)
{
    static const char *const names[ACT_OTHER + 1] = {
        "GATHER", "BUILD",  "BUILD+", "TRAIN",  "TRAIN+",
        "RESEARCH", "RESEARCH+", "CLAIM", "ITEM_USE", "HEAL", "OTHER"
    };
    unsigned idx = (k <= ACT_OTHER) ? (unsigned)k : (unsigned)ACT_OTHER;
    return names[idx];
}

/* 1234567 -> "1.23M"; 2400 -> "2.4k"; else integer. */
static void fmt_compact(uint64_t v, char *out, size_t sz)
{
    if (v >= 1000000ULL)      snprintf(out, sz, "%.2fM", (double)v / 1000000.0);
    else if (v >= 1000ULL)    snprintf(out, sz, "%.1fk", (double)v / 1000.0);
    else                      snprintf(out, sz, "%llu", (unsigned long long)v);
}

/* Signed integer with thousands separators: -150000 -> "-150,000". */
static void fmt_thousands(int64_t v, char *out, size_t sz)
{
    int64_t mag = v < 0 ? -v : v;
    char raw[32];
    snprintf(raw, sizeof raw, "%lld", (long long)mag);
    size_t n = strlen(raw), o = 0;
    if (v < 0 && o + 1 < sz) out[o++] = '-';
    for (size_t i = 0; i < n; ++i) {
        if (o + 2 >= sz) break;
        out[o++] = raw[i];
        size_t rem = n - 1 - i;
        if (rem && rem % 3 == 0 && o + 1 < sz) out[o++] = ',';
    }
    out[o] = 0;
}

static void build_resource_bar(const ResourceStock *r, char *out, size_t sz)
{
    char tmp[320]; size_t o = 0;
    for (int i = 0; i < RI_COUNT && (o + 24) < sizeof tmp; ++i) {
        char c0[16]; fmt_compact((uint64_t)res_at(r, i), c0, sizeof c0);
        int n = snprintf(tmp + o, sizeof tmp - o, "[%s]:%s ", kResLetter[i], c0);
        if (n > 0) o += (size_t)n;
    }
    snprintf(out, sz, "%s", tmp);
}

/* Largest-moved resource since the previous row: "-150,000 Timber". */
static void build_delta(const ResourceStock *r, char *out, size_t sz)
{
    if (!s_have_prev) { snprintf(out, sz, "-"); return; }
    int best = -1; int64_t bestMag = 0;
    for (int i = 0; i < RI_COUNT; ++i) {
        int64_t d  = res_at(r, i) - res_at(&s_prev, i);
        int64_t m  = d < 0 ? -d : d;
        if (m > bestMag) { bestMag = m; best = i; }
    }
    if (best < 0 || bestMag == 0) { snprintf(out, sz, "-"); return; }
    int64_t d = res_at(r, best) - res_at(&s_prev, best);
    char num[40]; fmt_thousands(d, num, sizeof num);
    snprintf(out, sz, "%s %s", num, kResName[best]);
}

static void stamp(char *out, size_t sz)
{
    time_t t = time(NULL);
    struct tm tmv;
#ifdef _WIN32
    localtime_s(&tmv, &t);
#else
    localtime_r(&t, &tmv);
#endif
    snprintf(out, sz, "%04d-%02d-%02d %02d:%02d:%02d",
             tmv.tm_year + 1900, tmv.tm_mon + 1, tmv.tm_mday,
             tmv.tm_hour, tmv.tm_min, tmv.tm_sec);
}

void ActivityLogOpen(Connection *c)
{
    ActivityLogClose(c);
    if (!c || !c->automation.activity_log) return;

    /* Resolve the path: explicit config first, else data_path, else cwd. */
    if (c->automation.activity_log_path[0]) {
        snprintf(s_path, sizeof s_path, "%s", c->automation.activity_log_path);
    } else if (c->bot.data_path[0]) {
        snprintf(s_path, sizeof s_path, "%s/activity_log.txt", c->bot.data_path);
    } else {
        snprintf(s_path, sizeof s_path, "activity_log.txt");
    }

    s_fp = fopen(s_path, "a");
    if (!s_fp) {
        LOGE("[ACTLOG] cannot open activity log '%s'\n", s_path);
        s_path[0] = 0;
        return;
    }

    s_seq = 0;
    s_have_prev = false;

    char hdr[200], st[40];
    stamp(st, sizeof st);
    snprintf(hdr, sizeof hdr,
             "=== LordsMobile Bot Activity Log   %s ===", st);
    fputs(hdr, s_fp); fputc('\n', s_fp);
    fputs("#seq  HH:MM:SS  |   power | kind       | action                  "
          "        | res-delta         | resource bar\n", s_fp);
    fputs("----------------+---------+-----------+"
          "--------------------+-------------------+----------------------"
          "-------------------\n", s_fp);

    res_set(&s_prev, &c->resources);
    s_have_prev = false;   /* first push baselines against real (loaded) stock */
    fflush(s_fp);
}

void ActivityLogPush(Connection *c, ActivityKind kind, const char *description)
{
    if (!s_fp || !c) return;

    char desc[72];
    {
        size_t n = strlen(description);
        if (n > 60) n = 60;
        memcpy(desc, description, n);
        desc[n] = 0;
    }
    char bar[320];   build_resource_bar(&c->resources, bar, sizeof bar);
    char delta[56];  build_delta(&c->resources, delta, sizeof delta);

    time_t t = time(NULL);
    struct tm tmv;
#ifdef _WIN32
    localtime_s(&tmv, &t);
#else
    localtime_r(&t, &tmv);
#endif

    char line[700];
    snprintf(line, sizeof line,
             "#%03u %02d:%02d:%02d | %8llu | %-9s | %-58s | %-18s | %s",
             (unsigned)++s_seq, tmv.tm_hour, tmv.tm_min, tmv.tm_sec,
             (unsigned long long)c->player.power,
             ActivityLogKindName(kind), desc, delta, bar);

    if (s_fp) {
        fputs(line, s_fp);
        fputc('\n', s_fp);
        fflush(s_fp);
    }
    LOGI("%s\n", line);

    res_set(&s_prev, &c->resources);
    s_have_prev = true;
}

void ActivityLogBaseline(Connection *c)
{
    if (!c) return;
    res_set(&s_prev, &c->resources);
    s_have_prev = true;
}

void ActivityLogClose(Connection *c)
{
    (void)c;
    if (s_fp) {
        fflush(s_fp);
        fclose(s_fp);
        s_fp = NULL;
    }
}