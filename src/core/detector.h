#ifndef DETECTOR_H
#define DETECTOR_H

#include "keymap.h"
#include <stdbool.h>
#include <stdint.h>

/* Suggestion for a key that appears to be bouncing */
typedef struct {
    int keycode;
    int rapid_count;           /* Number of rapid repeats detected */
    int suggested_delay_ms;    /* Suggested debounce delay */
} KeySuggestion;

/* Detection state per key */
typedef struct {
    double timestamps[64];     /* Circular buffer of recent keyDown timestamps */
    int ts_index;
    int ts_count;
} DetectorKeyState;

/* Detector instance */
typedef struct {
    DetectorKeyState key_states[N_VIRTUAL_KEY];
    double detection_window_sec;  /* Time window to look for rapid presses (default 60s) */
    double rapid_threshold_ms;    /* Max ms between key events to count as "rapid" (default 100ms) */
    int min_rapid_count;          /* Minimum rapid events to generate suggestion (default 5) */
} Detector;

/* Initialize the detector with default thresholds */
void detector_init(Detector *det);

/* Record a keyDown event (called from debouncer for every keyDown, even non-suppressed) */
void detector_record_event(Detector *det, int keycode, double timestamp);

/* Get suggestions for keys that appear to be bouncing.
 * Returns number of suggestions written to `out`, up to `max_suggestions`.
 */
int detector_get_suggestions(Detector *det, double current_time, KeySuggestion *out, int max_suggestions);

/* Reset detection state */
void detector_reset(Detector *det);

#endif /* DETECTOR_H */
