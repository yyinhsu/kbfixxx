#include "stats.h"
#include <string.h>

void stats_init(Stats *stats) {
    memset(stats, 0, sizeof(Stats));
}

void stats_record_suppressed(Stats *stats, int keycode) {
    if (keycode >= 0 && keycode < N_VIRTUAL_KEY) {
        stats->suppressed_count[keycode]++;
    }
}

void stats_record_event(Stats *stats, int keycode) {
    if (keycode >= 0 && keycode < N_VIRTUAL_KEY) {
        stats->total_events[keycode]++;
    }
}

void stats_reset(Stats *stats) {
    memset(stats, 0, sizeof(Stats));
}

uint64_t stats_get_suppressed(const Stats *stats, int keycode) {
    if (keycode >= 0 && keycode < N_VIRTUAL_KEY) {
        return stats->suppressed_count[keycode];
    }
    return 0;
}

uint64_t stats_get_total(const Stats *stats, int keycode) {
    if (keycode >= 0 && keycode < N_VIRTUAL_KEY) {
        return stats->total_events[keycode];
    }
    return 0;
}
