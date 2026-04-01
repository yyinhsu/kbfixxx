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
        db->key_states[i].last_key_up_ts = 0.0;
        db->key_states[i].last_key_down_ts = 0.0;
        db->key_states[i].last_event_type = 0;
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

    if (event_type == kCGEventKeyDown) {
        /* Auto-repeat events are never bounces — let them through */
        int64_t is_autorepeat = CGEventGetIntegerValueField(
            event, kCGKeyboardEventAutorepeat);
        if (!is_autorepeat) {
            /* Check 1: keyUp → keyDown bounce */
            if (ks->last_key_up_ts > 0.0) {
                double elapsed_ms = (now - ks->last_key_up_ts) * 1000.0;
                if (elapsed_ms < (double)kc->delay_ms) {
                    if (db->stats) {
                        stats_record_suppressed(db->stats, keycode);
                    }
                    emit_log(db, now, keycode, event_type, true, elapsed_ms);
                    return NULL;
                }
            }

            /* Check 2: keyDown → keyDown bounce (no keyUp in between) */
            if (ks->last_key_down_ts > 0.0) {
                double elapsed_ms = (now - ks->last_key_down_ts) * 1000.0;
                if (elapsed_ms < (double)kc->delay_ms) {
                    if (db->stats) {
                        stats_record_suppressed(db->stats, keycode);
                    }
                    emit_log(db, now, keycode, event_type, true, elapsed_ms);
                    return NULL;
                }
            }

            /* Legitimate press — record timestamp */
            ks->last_key_down_ts = now;
        }
    }

    /* Record keyUp timestamp */
    if (event_type == kCGEventKeyUp) {
        ks->last_key_up_ts = now;
    }

    ks->last_event_type = (int)event_type;
    emit_log(db, now, keycode, (int)event_type, false, 0);
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
        db->key_states[i].last_key_up_ts = 0.0;
        db->key_states[i].last_key_down_ts = 0.0;
        db->key_states[i].last_event_type = 0;
    }
}
