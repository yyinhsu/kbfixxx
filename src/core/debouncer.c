#include "debouncer.h"
#include "keymap.h"
#include <string.h>
#include <math.h>

/* Internal keyboard type IDs (butterfly keyboards) */
#define KEYBOARD_TYPE_INTERNAL_PRE2018 58
#define KEYBOARD_TYPE_INTERNAL_2018    59

static CGEventRef event_tap_callback(CGEventTapProxy proxy, CGEventType type,
                                     CGEventRef event, void *refcon);

void debouncer_init(Debouncer *db, Config *config, Stats *stats) {
    memset(db, 0, sizeof(Debouncer));
    db->config = config;
    db->stats = stats;
    db->disabled = false;
    db->event_tap = NULL;
    db->log_callback = NULL;
    db->log_context = NULL;

    for (int i = 0; i < N_VIRTUAL_KEY; i++) {
        db->key_states[i].last_event_type = 0;
        db->key_states[i].dismiss_next = false;
        db->key_states[i].bounce_counter = 0;
        db->key_states[i].timestamp_index = 0;
        memset(db->key_states[i].last_timestamps, 0, sizeof(db->key_states[i].last_timestamps));
    }
}

static void emit_log(Debouncer *db, double timestamp, int keycode,
                     int event_type, bool suppressed, double elapsed_ms) {
    if (db->log_callback) {
        LogEntry entry = {
            .timestamp = timestamp,
            .keycode = keycode,
            .event_type = event_type,
            .suppressed = suppressed,
            .elapsed_ms = elapsed_ms
        };
        db->log_callback(&entry, db->log_context);
    }
}

CGEventRef debouncer_filter_event(Debouncer *db, CGEventRef event) {
    if (db->disabled) return event;

    Config *cfg = db->config;

    /* Check keyboard type for ignore_external / ignore_internal */
    if (cfg->ignore_external_keyboard || cfg->ignore_internal_keyboard) {
        int64_t kb_type = CGEventGetIntegerValueField(event, kCGKeyboardEventKeyboardType);
        bool is_internal = (kb_type == KEYBOARD_TYPE_INTERNAL_PRE2018 ||
                           kb_type == KEYBOARD_TYPE_INTERNAL_2018);
        if (cfg->ignore_external_keyboard && !is_internal) return event;
        if (cfg->ignore_internal_keyboard && is_internal) return event;
    }

    CGKeyCode keycode = (CGKeyCode)CGEventGetIntegerValueField(event, kCGKeyboardEventKeycode);
    if (keycode >= N_VIRTUAL_KEY) return event;

    /* Skip unconfigured keys */
    KeyConfig *kc = &cfg->keys[keycode];
    if (!kc->enabled || kc->delay_ms <= 0) return event;

    CGEventType event_type = CGEventGetType(event);
    KeyState *ks = &db->key_states[keycode];

    /* Get current time */
    double now = CFAbsoluteTimeGetCurrent();

    /* Record event for auto-detection stats */
    if (event_type == kCGEventKeyDown && db->stats) {
        stats_record_event(db->stats, keycode);
    }

    /* Handle pending dismiss of keyUp after suppressed keyDown */
    if (ks->dismiss_next) {
        if (event_type == kCGEventKeyUp) {
            ks->dismiss_next = false;
            emit_log(db, now, keycode, event_type, true, 0);
            return NULL;
        }
    }

    if (event_type == kCGEventKeyDown) {
        double last_ts = ks->last_timestamps[ks->timestamp_index];

        if (last_ts > 0.0 && ks->last_event_type == kCGEventKeyUp) {
            double elapsed_ms = (now - last_ts) * 1000.0;

            /* NOTE: No auto-repeat guard needed here. Auto-repeat generates
             * consecutive keyDown events WITHOUT keyUp in between, so the
             * last_event_type == kCGEventKeyUp check above already excludes
             * auto-repeat. Ultra-fast keyUp→keyDown pairs (< 5ms) are
             * actually the signature of butterfly keyboard bounce. */

            if (elapsed_ms < (double)kc->delay_ms) {
                /* Bounce detected */
                ks->bounce_counter++;

                if (ks->bounce_counter <= kc->max_bounce_count) {
                    /* Suppress this bounce */
                    ks->dismiss_next = true;
                    if (db->stats) {
                        stats_record_suppressed(db->stats, keycode);
                    }
                    emit_log(db, now, keycode, event_type, true, elapsed_ms);
                    return NULL;
                }
                /* bounce_counter > max_bounce_count: user is genuinely pressing fast,
                 * let it through */
            }
        }

        /* Reset bounce counter when elapsed >= delay_ms (legitimate press) */
        if (last_ts > 0.0) {
            double elapsed_ms = (now - last_ts) * 1000.0;
            if (elapsed_ms >= (double)kc->delay_ms) {
                ks->bounce_counter = 0;
            }
        } else {
            ks->bounce_counter = 0;
        }
    }

    /* Record timestamp for this event */
    if (event_type == kCGEventKeyDown || event_type == kCGEventKeyUp) {
        ks->timestamp_index = (ks->timestamp_index + 1) % 4;
        ks->last_timestamps[ks->timestamp_index] = now;
        ks->last_event_type = (int)event_type;
    }

    emit_log(db, now, keycode, (int)event_type, false,
             ks->last_timestamps[ks->timestamp_index] > 0 ? 0 : 0);
    return event;
}

static CGEventRef event_tap_callback(CGEventTapProxy proxy, CGEventType type,
                                     CGEventRef event, void *refcon) {
    (void)proxy;
    Debouncer *db = (Debouncer *)refcon;

    /* If the event tap is disabled by the system, re-enable it */
    if (type == kCGEventTapDisabledByTimeout || type == kCGEventTapDisabledByUserInput) {
        if (db->event_tap) {
            CGEventTapEnable(db->event_tap, true);
        }
        return event;
    }

    return debouncer_filter_event(db, event);
}

bool debouncer_setup_event_tap(Debouncer *db) {
    CGEventMask mask = ((1 << kCGEventKeyDown) |
                        (1 << kCGEventKeyUp) |
                        (1 << kCGEventFlagsChanged));

    db->event_tap = CGEventTapCreate(kCGSessionEventTap,
                                     kCGHeadInsertEventTap,
                                     0,
                                     mask,
                                     event_tap_callback,
                                     db);

    if (!db->event_tap) {
        fprintf(stderr, "kbfixxx: failed to create event tap (accessibility permission required)\n");
        return false;
    }

    CFRunLoopSourceRef source = CFMachPortCreateRunLoopSource(kCFAllocatorDefault,
                                                              db->event_tap, 0);
    CFRunLoopAddSource(CFRunLoopGetCurrent(), source, kCFRunLoopCommonModes);
    CGEventTapEnable(db->event_tap, true);
    CFRelease(source);
    return true;
}

void debouncer_remove_event_tap(Debouncer *db) {
    if (!db->event_tap) return;
    CGEventTapEnable(db->event_tap, false);
    CFMachPortInvalidate(db->event_tap);
    CFRelease(db->event_tap);
    db->event_tap = NULL;
}

bool debouncer_event_tap_enabled(Debouncer *db) {
    if (!db->event_tap) return false;
    return CGEventTapIsEnabled(db->event_tap);
}

void debouncer_set_disabled(Debouncer *db, bool disabled) {
    db->disabled = disabled;
}

bool debouncer_is_disabled(Debouncer *db) {
    return db->disabled;
}

void debouncer_set_log_callback(Debouncer *db, LogCallback callback, void *context) {
    db->log_callback = callback;
    db->log_context = context;
}

void debouncer_reload_config(Debouncer *db) {
    /* Reset all key states when config changes */
    for (int i = 0; i < N_VIRTUAL_KEY; i++) {
        db->key_states[i].last_event_type = 0;
        db->key_states[i].dismiss_next = false;
        db->key_states[i].bounce_counter = 0;
        db->key_states[i].timestamp_index = 0;
        memset(db->key_states[i].last_timestamps, 0, sizeof(db->key_states[i].last_timestamps));
    }
}
