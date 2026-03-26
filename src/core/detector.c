#include "detector.h"
#include <string.h>
#include <math.h>

void detector_init(Detector *det) {
    memset(det, 0, sizeof(Detector));
    det->detection_window_sec = 60.0;   /* Look at last 60 seconds */
    det->rapid_threshold_ms = 100.0;    /* Events within 100ms count as rapid */
    det->min_rapid_count = 5;           /* Need at least 5 rapid events to suggest */
}

void detector_record_event(Detector *det, int keycode, double timestamp) {
    if (keycode < 0 || keycode >= N_VIRTUAL_KEY) return;

    DetectorKeyState *ks = &det->key_states[keycode];
    int idx = ks->ts_index % 64;
    ks->timestamps[idx] = timestamp;
    ks->ts_index = (ks->ts_index + 1) % 64;
    if (ks->ts_count < 64) ks->ts_count++;
}

int detector_get_suggestions(Detector *det, double current_time,
                             KeySuggestion *out, int max_suggestions) {
    int count = 0;

    for (int k = 0; k < N_VIRTUAL_KEY && count < max_suggestions; k++) {
        DetectorKeyState *ks = &det->key_states[k];
        if (ks->ts_count < 2) continue;

        int rapid_count = 0;
        double total_rapid_interval = 0.0;

        /* Walk through timestamps looking for rapid pairs within the detection window */
        for (int i = 1; i < ks->ts_count; i++) {
            int idx_curr = ((ks->ts_index - ks->ts_count + i) % 64 + 64) % 64;
            int idx_prev = ((ks->ts_index - ks->ts_count + i - 1) % 64 + 64) % 64;

            double ts_curr = ks->timestamps[idx_curr];
            double ts_prev = ks->timestamps[idx_prev];

            /* Skip entries outside the detection window */
            if (current_time - ts_curr > det->detection_window_sec) continue;
            if (ts_prev <= 0.0 || ts_curr <= 0.0) continue;

            double interval_ms = (ts_curr - ts_prev) * 1000.0;
            if (interval_ms > 0.0 && interval_ms < det->rapid_threshold_ms) {
                rapid_count++;
                total_rapid_interval += interval_ms;
            }
        }

        if (rapid_count >= det->min_rapid_count) {
            double avg_interval = total_rapid_interval / rapid_count;
            /* Suggest a delay slightly above the average rapid interval */
            int suggested = (int)(avg_interval * 1.5);
            if (suggested < 20) suggested = 20;
            if (suggested > 200) suggested = 200;

            out[count].keycode = k;
            out[count].rapid_count = rapid_count;
            out[count].suggested_delay_ms = suggested;
            count++;
        }
    }

    return count;
}

void detector_reset(Detector *det) {
    for (int i = 0; i < N_VIRTUAL_KEY; i++) {
        memset(&det->key_states[i], 0, sizeof(DetectorKeyState));
    }
}
