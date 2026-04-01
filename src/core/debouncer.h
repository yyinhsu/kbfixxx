#ifndef DEBOUNCER_H
#define DEBOUNCER_H

#include <ApplicationServices/ApplicationServices.h>
#include "config.h"
#include "stats.h"

/* Log entry for real-time event logging */
typedef struct {
    double timestamp;
    int keycode;
    int event_type;       /* kCGEventKeyDown or kCGEventKeyUp */
    bool suppressed;
    double elapsed_ms;
} LogEntry;

/* Callback type for log events */
typedef void (*LogCallback)(const LogEntry *entry, void *context);

/* Per-key runtime state */
typedef struct {
    double last_key_up_ts;       /* Timestamp of last keyUp (used as debounce reference) */
    int last_event_type;         /* Last recorded event type */
} KeyState;

/* Debouncer instance */
typedef struct {
    Config *config;
    Stats *stats;
    KeyState key_states[N_VIRTUAL_KEY];
    bool disabled;
    LogCallback log_callback;
    void *log_context;
    CFMachPortRef event_tap;
} Debouncer;

/* Initialize the debouncer with config and stats */
void debouncer_init(Debouncer *db, Config *config, Stats *stats);

/* Set up the CGEventTap. Returns true on success. */
bool debouncer_setup_event_tap(Debouncer *db);

/* Remove the event tap */
void debouncer_remove_event_tap(Debouncer *db);

/* Check if the event tap is still enabled */
bool debouncer_event_tap_enabled(Debouncer *db);

/* Core event filter function — called from the event tap callback */
CGEventRef debouncer_filter_event(Debouncer *db, CGEventRef event);

/* Enable/disable the debouncer */
void debouncer_set_disabled(Debouncer *db, bool disabled);
bool debouncer_is_disabled(Debouncer *db);

/* Set log callback for real-time logging */
void debouncer_set_log_callback(Debouncer *db, LogCallback callback, void *context);

/* Reload config (call after config_load) */
void debouncer_reload_config(Debouncer *db);

#endif /* DEBOUNCER_H */
