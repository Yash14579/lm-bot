#ifndef ACTIVITY_LOG_H
#define ACTIVITY_LOG_H

#include "connection.h"

/*
 * Bot activity log.
 *
 * Writes a tabular journal of every bot action to a local text file, one row
 * per line, mirroring the in-game "activity log" style:
 *
 *   #seq time  power#       action                         res-delta   resource bar
 *   #001 14:30:15  1752   GATHER  Gathering Ore (Lvl 5/2,475,000)  +2,475,000  [F]:0  [R]:1.2M  [W]:3.26M  [O]:0  [G]:45K
 *
 * - The resource-delta column is the change in the largest-moved resource
 *   since the previous row (a spend shows '-', an income '+'), computed by
 *   snapshotting c->resources on each push.
 * - The resource summary bar is the current live castle-stock snapshot.
 * - Pure local observability: opens/trims only a local file, never sends a
 *   packet, never allocates, and has zero gameplay consequence.
 */

/* Kind of bot action logged — maps to the game's activity-log verb. */
typedef enum {
    ACT_GATHER,        /* dispatching a gather march        */
    ACT_BUILD,         /* starting a build / upgrade        */
    ACT_BUILD_SPEED,   /* completing (speeding up) a build  */
    ACT_TRAIN,         /* starting a troop-train batch      */
    ACT_TRAIN_SPEED,   /* finishing (speeding up) a batch   */
    ACT_RESEARCH,      /* starting research                 */
    ACT_RESEARCH_FREE, /* completing / cancelling research  */
    ACT_CLAIM,         /* collecting any claimable reward   */
    ACT_ITEM_USE,      /* consuming a stock / currency item */
    ACT_HEAL,          /* hospital healing                  */
    ACT_OTHER
} ActivityKind;

const char *ActivityLogKindName(ActivityKind k);

/* Prepare logging: reads c->automation.activity_log (master enable) and
 * .activity_log_path, opens the file, prints a header, snapshots baseline.
 * Safe to call when disabled (then it is a no-op). */
void ActivityLogOpen(Connection *c);

/* Append one action row. Only honoured when enabled. The description is
 * copied into an internal fixed buffer (safe even for temporary strings). */
void ActivityLogPush(Connection *c, ActivityKind kind, const char *description);

/* Re-baseline the delta tracker without printing a row (call after the
 * resource snapshot changes for a reason not worth logging, e.g. at login). */
void ActivityLogBaseline(Connection *c);

/* Flush and close the file (call at shutdown). */
void ActivityLogClose(Connection *c);

#endif /* ACTIVITY_LOG_H */