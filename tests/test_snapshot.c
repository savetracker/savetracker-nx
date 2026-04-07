#include <string.h>

#include "snapshot.h"
#include "unity/unity.h"

static char path[512];

void setUp(void) {
    memset(path, 0, sizeof(path));
}

void tearDown(void) {}

// --- snapshot_decide ---

static void test_decide_no_history_returns_full(void) {
    SnapHistory h = {.last_full_mtime = 0, .last_any_mtime = 0};
    TEST_ASSERT_EQUAL_INT(SNAP_FULL, snapshot_decide(&h, 1000, 1800));
}

static void test_decide_null_history_returns_full(void) {
    TEST_ASSERT_EQUAL_INT(SNAP_FULL, snapshot_decide(NULL, 1000, 1800));
}

static void test_decide_recent_full_returns_patch(void) {
    // Last full was 100 seconds ago — well inside the 1800s window.
    SnapHistory h = {.last_full_mtime = 900, .last_any_mtime = 950};
    TEST_ASSERT_EQUAL_INT(SNAP_PATCH, snapshot_decide(&h, 1000, 1800));
}

static void test_decide_stale_full_returns_full(void) {
    // Last full was 2000 seconds ago — past the 1800s window, re-anchor.
    SnapHistory h = {.last_full_mtime = 0, .last_any_mtime = 0};
    h.last_full_mtime    = 1000;
    h.last_any_mtime     = 1000;
    TEST_ASSERT_EQUAL_INT(SNAP_FULL, snapshot_decide(&h, 3001, 1800));
}

static void test_decide_exactly_at_interval_returns_patch(void) {
    // Gap exactly equal to interval — still a patch (spec says "within").
    SnapHistory h = {.last_full_mtime = 1000, .last_any_mtime = 1100};
    TEST_ASSERT_EQUAL_INT(SNAP_PATCH, snapshot_decide(&h, 2800, 1800));
}

static void test_decide_just_past_interval_returns_full(void) {
    SnapHistory h = {.last_full_mtime = 1000, .last_any_mtime = 1100};
    TEST_ASSERT_EQUAL_INT(SNAP_FULL, snapshot_decide(&h, 2801, 1800));
}

static void test_decide_clock_rollback_returns_full(void) {
    SnapHistory h = {.last_full_mtime = 5000, .last_any_mtime = 6000};
    TEST_ASSERT_EQUAL_INT(SNAP_FULL, snapshot_decide(&h, 4000, 1800));
}

static void test_decide_same_mtime_returns_full(void) {
    // Duplicate mtime — treat as edge case, re-anchor with full.
    SnapHistory h = {.last_full_mtime = 5000, .last_any_mtime = 5500};
    TEST_ASSERT_EQUAL_INT(SNAP_FULL, snapshot_decide(&h, 5500, 1800));
}

static void test_decide_patch_after_patch(void) {
    // full at 1000, patch at 1200, now at 1400 — still within window → patch.
    SnapHistory h = {.last_full_mtime = 1000, .last_any_mtime = 1200};
    TEST_ASSERT_EQUAL_INT(SNAP_PATCH, snapshot_decide(&h, 1400, 1800));
}

// --- snapshot_build_path ---

static void test_build_path_full_snapshot(void) {
    TEST_ASSERT_TRUE(snapshot_build_path(path,
                                         sizeof(path),
                                         "0100_Borderlands 4",
                                         "save.dat",
                                         "20260405_143052_123",
                                         SNAP_FULL));
    TEST_ASSERT_EQUAL_STRING(
        "sdmc:/switch/savetracker/0100_Borderlands 4/save.dat/20260405_143052_123.snapshot",
        path);
}

static void test_build_path_patch(void) {
    TEST_ASSERT_TRUE(snapshot_build_path(path,
                                         sizeof(path),
                                         "0100",
                                         "2.sav",
                                         "20260405_143105_456",
                                         SNAP_PATCH));
    TEST_ASSERT_EQUAL_STRING(
        "sdmc:/switch/savetracker/0100/2.sav/20260405_143105_456.patch",
        path);
}

static void test_build_path_rejects_traversal_in_title(void) {
    TEST_ASSERT_FALSE(snapshot_build_path(path,
                                          sizeof(path),
                                          "../../evil",
                                          "save.dat",
                                          "20260405_143052_123",
                                          SNAP_FULL));
}

static void test_build_path_small_buffer(void) {
    char small[20];
    TEST_ASSERT_FALSE(snapshot_build_path(small,
                                          sizeof(small),
                                          "0100",
                                          "save.dat",
                                          "20260405_143052_123",
                                          SNAP_FULL));
}

static void test_build_path_null_inputs(void) {
    TEST_ASSERT_FALSE(snapshot_build_path(NULL, sizeof(path), "a", "b", "c", SNAP_FULL));
    TEST_ASSERT_FALSE(snapshot_build_path(path, 0, "a", "b", "c", SNAP_FULL));
    TEST_ASSERT_FALSE(snapshot_build_path(path, sizeof(path), NULL, "b", "c", SNAP_FULL));
    TEST_ASSERT_FALSE(snapshot_build_path(path, sizeof(path), "a", NULL, "c", SNAP_FULL));
    TEST_ASSERT_FALSE(snapshot_build_path(path, sizeof(path), "a", "b", NULL, SNAP_FULL));
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_decide_no_history_returns_full);
    RUN_TEST(test_decide_null_history_returns_full);
    RUN_TEST(test_decide_recent_full_returns_patch);
    RUN_TEST(test_decide_stale_full_returns_full);
    RUN_TEST(test_decide_exactly_at_interval_returns_patch);
    RUN_TEST(test_decide_just_past_interval_returns_full);
    RUN_TEST(test_decide_clock_rollback_returns_full);
    RUN_TEST(test_decide_same_mtime_returns_full);
    RUN_TEST(test_decide_patch_after_patch);
    RUN_TEST(test_build_path_full_snapshot);
    RUN_TEST(test_build_path_patch);
    RUN_TEST(test_build_path_rejects_traversal_in_title);
    RUN_TEST(test_build_path_small_buffer);
    RUN_TEST(test_build_path_null_inputs);
    return UNITY_END();
}
