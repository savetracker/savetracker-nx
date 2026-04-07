#include <string.h>

#include "safe_path.h"
#include "unity/unity.h"

static char out[512];

void setUp(void) {
    memset(out, 0, sizeof(out));
}

void tearDown(void) {}

static void test_accepts_exact_snapshots_root(void) {
    TEST_ASSERT_TRUE(safe_path_check("sdmc:/switch/savetracker", out, sizeof(out)));
    TEST_ASSERT_EQUAL_STRING("sdmc:/switch/savetracker", out);
}

static void test_accepts_exact_config_root(void) {
    TEST_ASSERT_TRUE(safe_path_check("sdmc:/config/savetracker", out, sizeof(out)));
    TEST_ASSERT_EQUAL_STRING("sdmc:/config/savetracker", out);
}

static void test_accepts_config_file(void) {
    TEST_ASSERT_TRUE(safe_path_check("sdmc:/config/savetracker/savetracker.conf", out, sizeof(out)));
    TEST_ASSERT_EQUAL_STRING("sdmc:/config/savetracker/savetracker.conf", out);
}

static void test_accepts_nested_path(void) {
    TEST_ASSERT_TRUE(safe_path_check("sdmc:/switch/savetracker/0100/save.dat/v1.snapshot", out, sizeof(out)));
    TEST_ASSERT_EQUAL_STRING("sdmc:/switch/savetracker/0100/save.dat/v1.snapshot", out);
}

static void test_normalizes_duplicate_slashes(void) {
    TEST_ASSERT_TRUE(safe_path_check("sdmc:/switch//savetracker///file.bin", out, sizeof(out)));
    TEST_ASSERT_EQUAL_STRING("sdmc:/switch/savetracker/file.bin", out);
}

static void test_normalizes_dot_segments(void) {
    TEST_ASSERT_TRUE(safe_path_check("sdmc:/switch/./savetracker/./file.bin", out, sizeof(out)));
    TEST_ASSERT_EQUAL_STRING("sdmc:/switch/savetracker/file.bin", out);
}

static void test_normalizes_parent_within_root(void) {
    TEST_ASSERT_TRUE(safe_path_check("sdmc:/switch/savetracker/sub/../file.bin", out, sizeof(out)));
    TEST_ASSERT_EQUAL_STRING("sdmc:/switch/savetracker/file.bin", out);
}

static void test_rejects_traversal_out_of_root(void) {
    TEST_ASSERT_FALSE(safe_path_check("sdmc:/switch/savetracker/../../Nintendo/evil", out, sizeof(out)));
}

static void test_rejects_save_directory(void) {
    TEST_ASSERT_FALSE(safe_path_check("sdmc:/Nintendo/save.dat", out, sizeof(out)));
}

static void test_rejects_arbitrary_sd_path(void) {
    TEST_ASSERT_FALSE(safe_path_check("sdmc:/other/file.txt", out, sizeof(out)));
}

static void test_rejects_wrong_mount_prefix(void) {
    TEST_ASSERT_FALSE(safe_path_check("sd:/switch/savetracker/file.bin", out, sizeof(out)));
}

static void test_rejects_relative_path(void) {
    TEST_ASSERT_FALSE(safe_path_check("switch/savetracker/file.bin", out, sizeof(out)));
}

static void test_rejects_empty_path(void) {
    TEST_ASSERT_FALSE(safe_path_check("", out, sizeof(out)));
}

static void test_rejects_root_prefix_extension(void) {
    // "sdmc:/switch/savetrackerrogue" must not match root "sdmc:/switch/savetracker".
    TEST_ASSERT_FALSE(safe_path_check("sdmc:/switch/savetrackerrogue/file.bin", out, sizeof(out)));
}

static void test_rejects_small_out_buffer(void) {
    char small[4];
    TEST_ASSERT_FALSE(safe_path_check("sdmc:/switch/savetracker/f.bin", small, sizeof(small)));
}

static void test_rejects_null_path(void) {
    TEST_ASSERT_FALSE(safe_path_check(NULL, out, sizeof(out)));
}

static void test_rejects_null_output(void) {
    TEST_ASSERT_FALSE(safe_path_check("sdmc:/switch/savetracker/f.bin", NULL, sizeof(out)));
}

static void test_rejects_zero_out_capacity(void) {
    TEST_ASSERT_FALSE(safe_path_check("sdmc:/switch/savetracker/f.bin", out, 0));
}

static void test_normalized_equals_root_exactly(void) {
    // Walk up and back down; should still land at the root.
    TEST_ASSERT_TRUE(safe_path_check("sdmc:/switch/savetracker/a/..", out, sizeof(out)));
    TEST_ASSERT_EQUAL_STRING("sdmc:/switch/savetracker", out);
}

static void test_deeply_nested_valid_path(void) {
    const char *deep = "sdmc:/switch/savetracker/a/b/c/d/e/f/g/h/file.snap";
    TEST_ASSERT_TRUE(safe_path_check(deep, out, sizeof(out)));
    TEST_ASSERT_EQUAL_STRING(deep, out);
}

static void test_parent_traversal_multiple_levels_inside(void) {
    TEST_ASSERT_TRUE(safe_path_check("sdmc:/switch/savetracker/a/b/c/../../file.bin", out, sizeof(out)));
    TEST_ASSERT_EQUAL_STRING("sdmc:/switch/savetracker/a/file.bin", out);
}

static void test_excess_parent_escapes_returns_false(void) {
    // Two ".." escapes past the root → rejected.
    TEST_ASSERT_FALSE(safe_path_check("sdmc:/switch/savetracker/../..", out, sizeof(out)));
}

static void test_rejects_nintendo_save_path(void) {
    TEST_ASSERT_FALSE(safe_path_check("sdmc:/Nintendo/save/0100000011B7C000", out, sizeof(out)));
}

static void test_rejects_sdmc_nintendo_path(void) {
    TEST_ASSERT_FALSE(safe_path_check("sdmc:/Nintendo/save.dat", out, sizeof(out)));
}

static void test_rejects_nand_system_path(void) {
    TEST_ASSERT_FALSE(safe_path_check("sdmc:/save/0100000011B7C000", out, sizeof(out)));
}

static void test_rejects_root_write(void) {
    TEST_ASSERT_FALSE(safe_path_check("sdmc:/important_file.bin", out, sizeof(out)));
}

static void test_rejects_atmosphere_path(void) {
    TEST_ASSERT_FALSE(safe_path_check("sdmc:/atmosphere/contents/whatever", out, sizeof(out)));
}

static void test_trailing_slash_preserved_as_segment(void) {
    // Trailing slash produces an empty segment which is skipped.
    TEST_ASSERT_TRUE(safe_path_check("sdmc:/switch/savetracker/sub/", out, sizeof(out)));
    TEST_ASSERT_EQUAL_STRING("sdmc:/switch/savetracker/sub", out);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_accepts_exact_snapshots_root);
    RUN_TEST(test_accepts_exact_config_root);
    RUN_TEST(test_accepts_config_file);
    RUN_TEST(test_accepts_nested_path);
    RUN_TEST(test_normalizes_duplicate_slashes);
    RUN_TEST(test_normalizes_dot_segments);
    RUN_TEST(test_normalizes_parent_within_root);
    RUN_TEST(test_rejects_traversal_out_of_root);
    RUN_TEST(test_rejects_save_directory);
    RUN_TEST(test_rejects_arbitrary_sd_path);
    RUN_TEST(test_rejects_wrong_mount_prefix);
    RUN_TEST(test_rejects_relative_path);
    RUN_TEST(test_rejects_empty_path);
    RUN_TEST(test_rejects_root_prefix_extension);
    RUN_TEST(test_rejects_small_out_buffer);
    RUN_TEST(test_rejects_null_path);
    RUN_TEST(test_rejects_null_output);
    RUN_TEST(test_rejects_zero_out_capacity);
    RUN_TEST(test_normalized_equals_root_exactly);
    RUN_TEST(test_deeply_nested_valid_path);
    RUN_TEST(test_parent_traversal_multiple_levels_inside);
    RUN_TEST(test_excess_parent_escapes_returns_false);
    RUN_TEST(test_rejects_nintendo_save_path);
    RUN_TEST(test_rejects_sdmc_nintendo_path);
    RUN_TEST(test_rejects_nand_system_path);
    RUN_TEST(test_rejects_root_write);
    RUN_TEST(test_rejects_atmosphere_path);
    RUN_TEST(test_trailing_slash_preserved_as_segment);
    return UNITY_END();
}
