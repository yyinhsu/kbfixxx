#include "config.h"
#include "cJSON.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void config_init_defaults(Config *cfg) {
    memset(cfg, 0, sizeof(Config));
    for (int i = 0; i < N_VIRTUAL_KEY; i++) {
        cfg->keys[i].delay_ms = 0;
        cfg->keys[i].max_bounce_count = 1;
        cfg->keys[i].enabled = false;
    }
    cfg->ignore_external_keyboard = false;
    cfg->ignore_internal_keyboard = false;
    cfg->config_path[0] = '\0';
}

const char* config_default_path(void) {
    static char path[1024];
    const char *home = getenv("HOME");
    if (!home) home = "/tmp";
    snprintf(path, sizeof(path), "%s/.config/kbfixxx/config.json", home);
    return path;
}

static char* read_file_contents(const char *path) {
    FILE *f = fopen(path, "r");
    if (!f) return NULL;

    fseek(f, 0, SEEK_END);
    long len = ftell(f);
    if (len <= 0 || len > 1024 * 1024) { /* Limit to 1MB */
        fclose(f);
        return NULL;
    }
    fseek(f, 0, SEEK_SET);

    char *buf = malloc((size_t)len + 1);
    if (!buf) {
        fclose(f);
        return NULL;
    }

    size_t read = fread(buf, 1, (size_t)len, f);
    buf[read] = '\0';
    fclose(f);
    return buf;
}

int config_load(Config *cfg, const char *path) {
    char *json_str = read_file_contents(path);
    if (!json_str) {
        fprintf(stderr, "kbfixxx: cannot read config file: %s\n", path);
        return -1;
    }

    cJSON *root = cJSON_Parse(json_str);
    free(json_str);
    if (!root) {
        fprintf(stderr, "kbfixxx: JSON parse error in %s: %s\n", path,
                cJSON_GetErrorPtr() ? cJSON_GetErrorPtr() : "unknown");
        return -1;
    }

    /* Only reset config after we know the file is valid */
    config_init_defaults(cfg);
    snprintf(cfg->config_path, sizeof(cfg->config_path), "%s", path);

    /* Parse global settings */
    cJSON *global = cJSON_GetObjectItem(root, "global");
    if (global) {
        cJSON *ign_ext = cJSON_GetObjectItem(global, "ignore_external_keyboard");
        if (cJSON_IsBool(ign_ext)) cfg->ignore_external_keyboard = cJSON_IsTrue(ign_ext);

        cJSON *ign_int = cJSON_GetObjectItem(global, "ignore_internal_keyboard");
        if (cJSON_IsBool(ign_int)) cfg->ignore_internal_keyboard = cJSON_IsTrue(ign_int);
    }

    /* Parse per-key settings */
    cJSON *keys = cJSON_GetObjectItem(root, "keys");
    if (keys && cJSON_IsObject(keys)) {
        cJSON *entry = NULL;
        cJSON_ArrayForEach(entry, keys) {
            const char *key_name = entry->string;
            if (!key_name) continue;

            /* Try to resolve key name to key code */
            int keycode = keycode_from_name(key_name);

            /* If not a name, try parsing as numeric key code */
            if (keycode < 0) {
                char *endptr;
                long code = strtol(key_name, &endptr, 10);
                if (*endptr == '\0' && code >= 0 && code < N_VIRTUAL_KEY) {
                    keycode = (int)code;
                }
            }

            if (keycode < 0 || keycode >= N_VIRTUAL_KEY) {
                fprintf(stderr, "kbfixxx: unknown key '%s' in config, skipping\n", key_name);
                continue;
            }

            if (!cJSON_IsObject(entry)) continue;

            cJSON *delay = cJSON_GetObjectItem(entry, "delay_ms");
            if (cJSON_IsNumber(delay)) {
                int val = delay->valueint;
                if (val < 0) val = 0;
                if (val > 10000) val = 10000;
                cfg->keys[keycode].delay_ms = val;
            }

            cJSON *bounce = cJSON_GetObjectItem(entry, "max_bounce_count");
            if (cJSON_IsNumber(bounce)) {
                int val = bounce->valueint;
                if (val < 1) val = 1;
                if (val > 10) val = 10;
                cfg->keys[keycode].max_bounce_count = val;
            }

            cJSON *enabled = cJSON_GetObjectItem(entry, "enabled");
            if (cJSON_IsBool(enabled)) {
                cfg->keys[keycode].enabled = cJSON_IsTrue(enabled);
            } else {
                /* If enabled is not specified, enable if delay_ms > 0 */
                cfg->keys[keycode].enabled = (cfg->keys[keycode].delay_ms > 0);
            }
        }
    }

    cJSON_Delete(root);
    return 0;
}

int config_save(const Config *cfg, const char *path) {
    cJSON *root = cJSON_CreateObject();
    if (!root) return -1;

    /* Global settings */
    cJSON *global = cJSON_AddObjectToObject(root, "global");
    cJSON_AddBoolToObject(global, "ignore_external_keyboard", cfg->ignore_external_keyboard);
    cJSON_AddBoolToObject(global, "ignore_internal_keyboard", cfg->ignore_internal_keyboard);

    /* Per-key settings — only write keys that have delay_ms > 0 */
    cJSON *keys = cJSON_AddObjectToObject(root, "keys");
    for (int i = 0; i < N_VIRTUAL_KEY; i++) {
        if (cfg->keys[i].delay_ms <= 0 && !cfg->keys[i].enabled) continue;

        const char *name = keycode_to_name(i);
        char num_buf[8];
        if (strcmp(name, "unknown") == 0) {
            /* Use numeric key code as string */
            snprintf(num_buf, sizeof(num_buf), "%d", i);
            name = num_buf;
        }

        cJSON *key_obj = cJSON_AddObjectToObject(keys, name);
        cJSON_AddNumberToObject(key_obj, "delay_ms", cfg->keys[i].delay_ms);
        cJSON_AddNumberToObject(key_obj, "max_bounce_count", cfg->keys[i].max_bounce_count);
        cJSON_AddBoolToObject(key_obj, "enabled", cfg->keys[i].enabled);
    }

    char *json_str = cJSON_Print(root);
    cJSON_Delete(root);
    if (!json_str) return -1;

    FILE *f = fopen(path, "w");
    if (!f) {
        cJSON_free(json_str);
        fprintf(stderr, "kbfixxx: cannot write config file: %s\n", path);
        return -1;
    }

    fprintf(f, "%s\n", json_str);
    fclose(f);
    cJSON_free(json_str);
    return 0;
}
