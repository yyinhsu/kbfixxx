#ifndef STATS_H
#define STATS_H

#include "keymap.h"
#include <stdint.h>

/* Per-key statistics (in-memory only, resets on app restart) */
typedef struct {
    uint64_t suppressed_count[N_VIRTUAL_KEY];  /* Number of suppressed events per key */
    uint64_t total_events[N_VIRTUAL_KEY];      /* Total keyDown events per key */
} Stats;

/* Initialize stats to zero */
void stats_init(Stats *stats);

/* Record a suppressed event for a key */
void stats_record_suppressed(Stats *stats, int keycode);

/* Record a total keyDown event for a key */
void stats_record_event(Stats *stats, int keycode);

/* Reset all statistics */
void stats_reset(Stats *stats);

/* Get suppressed count for a key */
uint64_t stats_get_suppressed(const Stats *stats, int keycode);

/* Get total event count for a key */
uint64_t stats_get_total(const Stats *stats, int keycode);

#endif /* STATS_H */
