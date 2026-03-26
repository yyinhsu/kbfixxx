/*
 * test_debouncer.c — Unit tests for the core debounce logic
 *
 * We test the debounce algorithm directly without CGEventTap
 * by simulating key states and timestamp sequences.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <math.h>
#include "config.h"
#include "stats.h"
#include "detector.h"
#include "keymap.h"

/* Since we can't use CGEventRef in a pure C test, we test
 * the config, stats, detector, and keymap modules directly. */

static int tests_passed = 0;
static int tests_failed = 0;

#define TEST(name) \
    do { printf("  TEST: %s ... ", #name); } while(0)

#define PASS() \
    do { printf("PASS\n"); tests_passed++; } while(0)

#define FAIL(msg) \
    do { printf("FAIL: %s\n", msg); tests_failed++; } while(0)

#define ASSERT_EQ(a, b, msg) \
    do { if ((a) != (b)) { FAIL(msg); return; } } while(0)

#define ASSERT_STR_EQ(a, b, msg) \
    do { if (strcmp((a), (b)) != 0) { FAIL(msg); return; } } while(0)

#define ASSERT_TRUE(cond, msg) \
    do { if (!(cond)) { FAIL(msg); return; } } while(0)

/* Test keymap name resolution */
static void test_keymap(void) {
    printf("\n=== Keymap Tests ===\n");

    TEST(keycode_to_name_valid);
    ASSERT_STR_EQ(keycode_to_name(0x00), "a", "key 0x00 should be 'a'");
    ASSERT_STR_EQ(keycode_to_name(0x0B), "b", "key 0x0B should be 'b'");
    ASSERT_STR_EQ(keycode_to_name(0x31), "space", "key 0x31 should be 'space'");
    PASS();

    TEST(keycode_to_name_unknown);
    ASSERT_STR_EQ(keycode_to_name(0x34), "unknown", "unmapped key should be 'unknown'");
    ASSERT_STR_EQ(keycode_to_name(200), "unknown", "out of range key should be 'unknown'");
    PASS();

    TEST(keycode_from_name_valid);
    ASSERT_EQ(keycode_from_name("a"), 0x00, "name 'a' should resolve to 0x00");
    ASSERT_EQ(keycode_from_name("b"), 0x0B, "name 'b' should resolve to 0x0B");
    ASSERT_EQ(keycode_from_name("space"), 0x31, "name 'space' should resolve to 0x31");
    ASSERT_EQ(keycode_from_name("return"), 0x24, "name 'return' should resolve to 0x24");
    PASS();

    TEST(keycode_from_name_invalid);
    ASSERT_EQ(keycode_from_name("nonexistent"), -1, "unknown name should return -1");
    ASSERT_EQ(keycode_from_name(NULL), -1, "NULL name should return -1");
    PASS();
}

/* Test config init and defaults */
static void test_config(void) {
    printf("\n=== Config Tests ===\n");

    TEST(config_init_defaults);
    Config cfg;
    config_init_defaults(&cfg);
    ASSERT_TRUE(!cfg.ignore_external_keyboard, "ignore_external should be false");
    ASSERT_TRUE(!cfg.ignore_internal_keyboard, "ignore_internal should be false");
    for (int i = 0; i < N_VIRTUAL_KEY; i++) {
        ASSERT_EQ(cfg.keys[i].delay_ms, 0, "delay should be 0");
        ASSERT_EQ(cfg.keys[i].max_bounce_count, 1, "max_bounce should be 1");
        ASSERT_TRUE(!cfg.keys[i].enabled, "enabled should be false");
    }
    PASS();

    TEST(config_load_save_roundtrip);
    Config cfg2;
    config_init_defaults(&cfg2);
    cfg2.keys[0x0B].delay_ms = 60;       /* b */
    cfg2.keys[0x0B].max_bounce_count = 2;
    cfg2.keys[0x0B].enabled = true;
    cfg2.keys[0x31].delay_ms = 80;       /* space */
    cfg2.keys[0x31].max_bounce_count = 3;
    cfg2.keys[0x31].enabled = true;
    cfg2.ignore_external_keyboard = true;

    const char *tmp_path = "/tmp/kbfixxx_test_config.json";
    ASSERT_EQ(config_save(&cfg2, tmp_path), 0, "save should succeed");

    Config cfg3;
    ASSERT_EQ(config_load(&cfg3, tmp_path), 0, "load should succeed");
    ASSERT_EQ(cfg3.keys[0x0B].delay_ms, 60, "b delay should be 60");
    ASSERT_EQ(cfg3.keys[0x0B].max_bounce_count, 2, "b bounce count should be 2");
    ASSERT_TRUE(cfg3.keys[0x0B].enabled, "b should be enabled");
    ASSERT_EQ(cfg3.keys[0x31].delay_ms, 80, "space delay should be 80");
    ASSERT_EQ(cfg3.keys[0x31].max_bounce_count, 3, "space bounce count should be 3");
    ASSERT_TRUE(cfg3.ignore_external_keyboard, "ignore_external should be true");

    remove(tmp_path);
    PASS();

    TEST(config_load_nonexistent);
    Config cfg4;
    ASSERT_EQ(config_load(&cfg4, "/tmp/nonexistent_kbfixxx.json"), -1, "loading nonexistent should fail");
    PASS();
}

/* Test stats */
static void test_stats(void) {
    printf("\n=== Stats Tests ===\n");

    TEST(stats_init_and_record);
    Stats stats;
    stats_init(&stats);
    ASSERT_EQ(stats_get_suppressed(&stats, 0x0B), (uint64_t)0, "initial suppressed should be 0");
    ASSERT_EQ(stats_get_total(&stats, 0x0B), (uint64_t)0, "initial total should be 0");

    stats_record_event(&stats, 0x0B);
    stats_record_event(&stats, 0x0B);
    stats_record_suppressed(&stats, 0x0B);
    ASSERT_EQ(stats_get_total(&stats, 0x0B), (uint64_t)2, "total should be 2");
    ASSERT_EQ(stats_get_suppressed(&stats, 0x0B), (uint64_t)1, "suppressed should be 1");
    PASS();

    TEST(stats_reset);
    stats_reset(&stats);
    ASSERT_EQ(stats_get_suppressed(&stats, 0x0B), (uint64_t)0, "after reset suppressed should be 0");
    ASSERT_EQ(stats_get_total(&stats, 0x0B), (uint64_t)0, "after reset total should be 0");
    PASS();

    TEST(stats_boundary);
    ASSERT_EQ(stats_get_total(&stats, -1), (uint64_t)0, "negative keycode should return 0");
    ASSERT_EQ(stats_get_total(&stats, N_VIRTUAL_KEY), (uint64_t)0, "out of range keycode should return 0");
    PASS();
}

/* Test detector */
static void test_detector(void) {
    printf("\n=== Detector Tests ===\n");

    TEST(detector_no_suggestions_initially);
    Detector det;
    detector_init(&det);
    KeySuggestion suggestions[10];
    int count = detector_get_suggestions(&det, 1000.0, suggestions, 10);
    ASSERT_EQ(count, 0, "no suggestions initially");
    PASS();

    TEST(detector_detects_rapid_presses);
    detector_reset(&det);
    det.rapid_threshold_ms = 100.0;
    det.min_rapid_count = 3;
    det.detection_window_sec = 60.0;

    /* Simulate rapid presses on key 'b' (0x0B) */
    double base = 1000.0;
    for (int i = 0; i < 10; i++) {
        detector_record_event(&det, 0x0B, base + i * 0.050);  /* 50ms apart */
    }

    count = detector_get_suggestions(&det, base + 1.0, suggestions, 10);
    ASSERT_TRUE(count >= 1, "should detect rapid presses");
    ASSERT_EQ(suggestions[0].keycode, 0x0B, "should be key b");
    ASSERT_TRUE(suggestions[0].suggested_delay_ms >= 20, "suggested delay should be reasonable");
    PASS();

    TEST(detector_ignores_slow_presses);
    detector_reset(&det);
    base = 2000.0;
    for (int i = 0; i < 10; i++) {
        detector_record_event(&det, 0x0B, base + i * 0.500);  /* 500ms apart — normal typing */
    }
    count = detector_get_suggestions(&det, base + 10.0, suggestions, 10);
    ASSERT_EQ(count, 0, "should not detect normal typing as rapid");
    PASS();
}

int main(void) {
    printf("kbfixxx — Unit Tests\n");
    printf("====================\n");

    test_keymap();
    test_config();
    test_stats();
    test_detector();

    printf("\n====================\n");
    printf("Results: %d passed, %d failed\n", tests_passed, tests_failed);

    return tests_failed > 0 ? 1 : 0;
}
