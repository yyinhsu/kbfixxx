#ifndef CONFIG_H
#define CONFIG_H

#include <stdbool.h>
#include "keymap.h"

/* Per-key configuration */
typedef struct {
    int delay_ms;           /* Debounce delay in milliseconds */
    int max_bounce_count;   /* How many consecutive bounces to suppress (default 1) */
    bool enabled;           /* Whether debouncing is active for this key */
} KeyConfig;

/* Global configuration */
typedef struct {
    bool ignore_external_keyboard;
    bool ignore_internal_keyboard;
    KeyConfig keys[N_VIRTUAL_KEY];
    char config_path[1024];
} Config;

/* Load config from JSON file. Returns 0 on success, -1 on error. */
int config_load(Config *cfg, const char *path);

/* Save current config to JSON file. Returns 0 on success, -1 on error. */
int config_save(const Config *cfg, const char *path);

/* Initialize config with defaults (all keys disabled, delay 0) */
void config_init_defaults(Config *cfg);

/* Get default config file path (~/.config/kbfixxx/config.json) */
const char* config_default_path(void);

#endif /* CONFIG_H */
