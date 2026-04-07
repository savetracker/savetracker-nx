#include <string.h>

#include "config.h"
#include "unity/unity.h"

static Config cfg;

void setUp(void) {
    config_init_defaults(&cfg);
}

void tearDown(void) {}

static bool parse(const char *text) {
    return config_parse(&cfg, text, strlen(text));
}

static void test_defaults(void) {
    TEST_ASSERT_EQUAL_UINT32(90, cfg.title_check_secs);
    TEST_ASSERT_EQUAL_UINT32(15, cfg.save_check_secs);
    TEST_ASSERT_EQUAL_UINT32(90, cfg.idle_secs);
    TEST_ASSERT_EQUAL_UINT32(1800, cfg.full_snapshot_secs);
    TEST_ASSERT_EQUAL_UINT32(3, cfg.write_delay_secs);
    TEST_ASSERT_EQUAL_INT(LED_COLOR_YELLOW, cfg.led_new_title.color);
    TEST_ASSERT_EQUAL_UINT32(3, cfg.led_new_title.duration_secs);
    TEST_ASSERT_EQUAL_INT(LED_COLOR_RED, cfg.led_error.color);
    TEST_ASSERT_EQUAL_UINT32(2, cfg.led_error.duration_secs);
    TEST_ASSERT_FALSE(cfg.pause_on_battery);
}

static void test_empty_text_keeps_defaults(void) {
    TEST_ASSERT_TRUE(parse(""));
    TEST_ASSERT_EQUAL_UINT32(90, cfg.title_check_secs);
}

static void test_blank_lines_and_comments_ignored(void) {
    TEST_ASSERT_TRUE(parse("\n\n# this is a comment\n  # indented comment\n\n"));
    TEST_ASSERT_EQUAL_UINT32(90, cfg.title_check_secs);
}

static void test_override_interval(void) {
    TEST_ASSERT_TRUE(parse("title_check_secs = 60\n"));
    TEST_ASSERT_EQUAL_UINT32(60, cfg.title_check_secs);
}

static void test_override_multiple_keys(void) {
    const char *text =
        "title_check_secs=30\n"
        "save_check_secs=5\n"
        "full_snapshot_secs=600\n";
    TEST_ASSERT_TRUE(parse(text));
    TEST_ASSERT_EQUAL_UINT32(30, cfg.title_check_secs);
    TEST_ASSERT_EQUAL_UINT32(5, cfg.save_check_secs);
    TEST_ASSERT_EQUAL_UINT32(600, cfg.full_snapshot_secs);
}

static void test_whitespace_stripped(void) {
    TEST_ASSERT_TRUE(parse("   title_check_secs   =   120   \n"));
    TEST_ASSERT_EQUAL_UINT32(120, cfg.title_check_secs);
}

static void test_crlf_line_endings(void) {
    TEST_ASSERT_TRUE(parse("title_check_secs = 45\r\nsave_check_secs = 10\r\n"));
    TEST_ASSERT_EQUAL_UINT32(45, cfg.title_check_secs);
    TEST_ASSERT_EQUAL_UINT32(10, cfg.save_check_secs);
}

static void test_unknown_key_is_ignored(void) {
    TEST_ASSERT_TRUE(parse("some_future_key = hello\ntitle_check_secs = 42\n"));
    TEST_ASSERT_EQUAL_UINT32(42, cfg.title_check_secs);
}

static void test_malformed_line_fails(void) {
    TEST_ASSERT_FALSE(parse("no_equals_sign_here\n"));
}

static void test_empty_key_fails(void) {
    TEST_ASSERT_FALSE(parse(" = 10\n"));
}

static void test_led_color_parsing(void) {
    TEST_ASSERT_TRUE(parse("led_new_title_color = green\n"));
    TEST_ASSERT_EQUAL_INT(LED_COLOR_GREEN, cfg.led_new_title.color);
}

static void test_led_color_case_insensitive(void) {
    TEST_ASSERT_TRUE(parse("led_error_color = BLUE\n"));
    TEST_ASSERT_EQUAL_INT(LED_COLOR_BLUE, cfg.led_error.color);
}

static void test_led_color_invalid_fails(void) {
    TEST_ASSERT_FALSE(parse("led_new_title_color = purple\n"));
}

static void test_led_disable_via_duration_zero(void) {
    TEST_ASSERT_TRUE(parse("led_error_duration = 0\n"));
    TEST_ASSERT_EQUAL_UINT32(0, cfg.led_error.duration_secs);
}

static void test_bool_true_forms(void) {
    TEST_ASSERT_TRUE(parse("pause_on_battery = true\n"));
    TEST_ASSERT_TRUE(cfg.pause_on_battery);

    cfg.pause_on_battery = false;
    TEST_ASSERT_TRUE(parse("pause_on_battery = yes\n"));
    TEST_ASSERT_TRUE(cfg.pause_on_battery);

    cfg.pause_on_battery = false;
    TEST_ASSERT_TRUE(parse("pause_on_battery = 1\n"));
    TEST_ASSERT_TRUE(cfg.pause_on_battery);
}

static void test_bool_false_forms(void) {
    cfg.pause_on_battery = true;
    TEST_ASSERT_TRUE(parse("pause_on_battery = false\n"));
    TEST_ASSERT_FALSE(cfg.pause_on_battery);

    cfg.pause_on_battery = true;
    TEST_ASSERT_TRUE(parse("pause_on_battery = no\n"));
    TEST_ASSERT_FALSE(cfg.pause_on_battery);

    cfg.pause_on_battery = true;
    TEST_ASSERT_TRUE(parse("pause_on_battery = 0\n"));
    TEST_ASSERT_FALSE(cfg.pause_on_battery);
}

static void test_bool_invalid_fails(void) {
    TEST_ASSERT_FALSE(parse("pause_on_battery = maybe\n"));
}

static void test_uint_invalid_fails(void) {
    TEST_ASSERT_FALSE(parse("title_check_secs = -5\n"));
}

static void test_uint_non_numeric_fails(void) {
    TEST_ASSERT_FALSE(parse("title_check_secs = abc\n"));
}

static void test_last_line_without_newline(void) {
    TEST_ASSERT_TRUE(parse("title_check_secs = 30"));
    TEST_ASSERT_EQUAL_UINT32(30, cfg.title_check_secs);
}

static void test_realistic_full_config(void) {
    const char *text =
        "# savetracker-nx config\n"
        "\n"
        "title_check_secs = 60\n"
        "save_check_secs = 10\n"
        "idle_secs = 120\n"
        "full_snapshot_secs = 900\n"
        "write_delay_secs = 5\n"
        "\n"
        "led_new_title_color = cyan\n"
        "led_new_title_duration = 5\n"
        "led_error_color = red\n"
        "led_error_duration = 2\n"
        "\n"
        "pause_on_battery = true\n";

    TEST_ASSERT_TRUE(parse(text));
    TEST_ASSERT_EQUAL_UINT32(60, cfg.title_check_secs);
    TEST_ASSERT_EQUAL_UINT32(10, cfg.save_check_secs);
    TEST_ASSERT_EQUAL_UINT32(120, cfg.idle_secs);
    TEST_ASSERT_EQUAL_UINT32(900, cfg.full_snapshot_secs);
    TEST_ASSERT_EQUAL_UINT32(5, cfg.write_delay_secs);
    TEST_ASSERT_EQUAL_INT(LED_COLOR_CYAN, cfg.led_new_title.color);
    TEST_ASSERT_EQUAL_UINT32(5, cfg.led_new_title.duration_secs);
    TEST_ASSERT_EQUAL_INT(LED_COLOR_RED, cfg.led_error.color);
    TEST_ASSERT_EQUAL_UINT32(2, cfg.led_error.duration_secs);
    TEST_ASSERT_TRUE(cfg.pause_on_battery);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_defaults);
    RUN_TEST(test_empty_text_keeps_defaults);
    RUN_TEST(test_blank_lines_and_comments_ignored);
    RUN_TEST(test_override_interval);
    RUN_TEST(test_override_multiple_keys);
    RUN_TEST(test_whitespace_stripped);
    RUN_TEST(test_crlf_line_endings);
    RUN_TEST(test_unknown_key_is_ignored);
    RUN_TEST(test_malformed_line_fails);
    RUN_TEST(test_empty_key_fails);
    RUN_TEST(test_led_color_parsing);
    RUN_TEST(test_led_color_case_insensitive);
    RUN_TEST(test_led_color_invalid_fails);
    RUN_TEST(test_led_disable_via_duration_zero);
    RUN_TEST(test_bool_true_forms);
    RUN_TEST(test_bool_false_forms);
    RUN_TEST(test_bool_invalid_fails);
    RUN_TEST(test_uint_invalid_fails);
    RUN_TEST(test_uint_non_numeric_fails);
    RUN_TEST(test_last_line_without_newline);
    RUN_TEST(test_realistic_full_config);
    return UNITY_END();
}
