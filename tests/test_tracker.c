#include <stdio.h>
#include <string.h>

#include "tracker.h"
#include "unity/unity.h"

static Tracker t;
static Config  cfg;

void setUp(void) {
    tracker_init(&t);
    config_init_defaults(&cfg);
}

void tearDown(void) {
    tracker_clear(&t);
}

static void test_no_title_on_first_tick(void) {
    TitleChange c = tracker_update_title(&t, &cfg, 0, false, 0);
    TEST_ASSERT_EQUAL_INT(TITLE_NONE, c);
    TEST_ASSERT_FALSE(t.active);
}

static void test_new_title_detected(void) {
    TitleChange c = tracker_update_title(&t, &cfg, 0, true, 0x1234);
    TEST_ASSERT_EQUAL_INT(TITLE_NEW, c);
    TEST_ASSERT_TRUE(t.active);
    TEST_ASSERT_EQUAL_UINT64(0x1234, t.title_id);
}

static void test_same_title_after_detection(void) {
    tracker_update_title(&t, &cfg, 0, true, 0x1234);
    TitleChange c = tracker_update_title(&t, &cfg, 15, true, 0x1234);
    TEST_ASSERT_EQUAL_INT(TITLE_SAME, c);
}

static void test_same_title_after_interval(void) {
    tracker_update_title(&t, &cfg, 0, true, 0x1234);
    TitleChange c = tracker_update_title(&t, &cfg, cfg.title_check_secs, true, 0x1234);
    TEST_ASSERT_EQUAL_INT(TITLE_SAME, c);
}

static void test_title_gone(void) {
    tracker_update_title(&t, &cfg, 0, true, 0x1234);
    TitleChange c = tracker_update_title(&t, &cfg, cfg.title_check_secs, false, 0);
    TEST_ASSERT_EQUAL_INT(TITLE_GONE, c);
    TEST_ASSERT_FALSE(t.active);
}

static void test_title_changed(void) {
    tracker_update_title(&t, &cfg, 0, true, 0x1234);
    TitleChange c = tracker_update_title(&t, &cfg, cfg.title_check_secs, true, 0x5678);
    TEST_ASSERT_EQUAL_INT(TITLE_NEW, c);
    TEST_ASSERT_EQUAL_UINT64(0x5678, t.title_id);
}

static void test_title_reappears_after_gone(void) {
    tracker_update_title(&t, &cfg, 0, true, 0x1234);
    tracker_update_title(&t, &cfg, cfg.title_check_secs, false, 0);
    TitleChange c = tracker_update_title(&t, &cfg, cfg.title_check_secs * 2, true, 0x1234);
    TEST_ASSERT_EQUAL_INT(TITLE_NEW, c);
}

static void test_set_dir_name(void) {
    tracker_update_title(&t, &cfg, 0, true, 0x1234);
    tracker_set_dir_name(&t, "0000000000001234_TestGame");
    TEST_ASSERT_EQUAL_STRING("0000000000001234_TestGame", t.dir_name);
}

static void test_get_file_creates_new(void) {
    FileState *fs = tracker_get_file(&t, "save.dat");
    TEST_ASSERT_NOT_NULL(fs);
    TEST_ASSERT_EQUAL_STRING("save.dat", fs->name);
    TEST_ASSERT_FALSE(fs->valid);
    TEST_ASSERT_EQUAL_UINT32(1, t.file_count);
}

static void test_get_file_returns_existing(void) {
    FileState *a = tracker_get_file(&t, "save.dat");
    FileState *b = tracker_get_file(&t, "save.dat");
    TEST_ASSERT_EQUAL_PTR(a, b);
    TEST_ASSERT_EQUAL_UINT32(1, t.file_count);
}

static void test_get_file_grows_dynamically(void) {
    for (int i = 0; i < 20; i++) {
        char name[32];
        snprintf(name, sizeof(name), "file%d.dat", i);
        TEST_ASSERT_NOT_NULL(tracker_get_file(&t, name));
    }
    TEST_ASSERT_EQUAL_UINT32(20, t.file_count);
}

static void test_needs_read_first_time(void) {
    FileState *fs = tracker_get_file(&t, "save.dat");
    TEST_ASSERT_TRUE(tracker_file_needs_read(fs, 1000));
}

static void test_needs_read_mtime_advanced(void) {
    FileState *fs = tracker_get_file(&t, "save.dat");
    tracker_file_commit(fs, 1000, "sdmc:/switch/savetracker/t/save.dat/snap.snapshot", SNAP_FULL);
    TEST_ASSERT_TRUE(tracker_file_needs_read(fs, 1001));
}

static void test_no_read_same_mtime(void) {
    FileState *fs = tracker_get_file(&t, "save.dat");
    tracker_file_commit(fs, 1000, "sdmc:/switch/savetracker/t/save.dat/snap.snapshot", SNAP_FULL);
    TEST_ASSERT_FALSE(tracker_file_needs_read(fs, 1000));
}

static void test_needs_read_mtime_unavailable(void) {
    FileState *fs = tracker_get_file(&t, "save.dat");
    tracker_file_commit(fs, 1000, "sdmc:/switch/savetracker/t/save.dat/snap.snapshot", SNAP_FULL);
    TEST_ASSERT_TRUE(tracker_file_needs_read(fs, 0));
}

static void test_delay_not_elapsed_immediately(void) {
    FileState *fs = tracker_get_file(&t, "save.dat");
    tracker_file_mark_changed(fs, 100);
    TEST_ASSERT_FALSE(tracker_file_delay_elapsed(fs, &cfg, 100));
}

static void test_delay_elapsed_after_wait(void) {
    FileState *fs = tracker_get_file(&t, "save.dat");
    tracker_file_mark_changed(fs, 100);
    TEST_ASSERT_TRUE(tracker_file_delay_elapsed(fs, &cfg, 100 + cfg.write_delay_secs));
}

static void test_mark_changed_only_records_first(void) {
    FileState *fs = tracker_get_file(&t, "save.dat");
    tracker_file_mark_changed(fs, 100);
    tracker_file_mark_changed(fs, 200);
    TEST_ASSERT_TRUE(tracker_file_delay_elapsed(fs, &cfg, 100 + cfg.write_delay_secs));
}

static void test_commit_updates_state(void) {
    FileState *fs = tracker_get_file(&t, "save.dat");
    tracker_file_commit(fs, 5000, "sdmc:/switch/savetracker/t/save.dat/20260406.snapshot", SNAP_FULL);

    TEST_ASSERT_TRUE(fs->valid);
    TEST_ASSERT_EQUAL_UINT64(5000, fs->last_seen_mtime);
    TEST_ASSERT_EQUAL_UINT64(5000, fs->last_full_mtime);
    TEST_ASSERT_EQUAL_UINT64(5000, fs->last_any_mtime);
    TEST_ASSERT_EQUAL_UINT64(0, fs->change_detected_at);
    TEST_ASSERT_EQUAL_STRING("sdmc:/switch/savetracker/t/save.dat/20260406.snapshot", fs->last_full_path);
}

static void test_commit_patch_does_not_update_full_path(void) {
    FileState *fs = tracker_get_file(&t, "save.dat");
    tracker_file_commit(fs, 1000, "sdmc:/switch/savetracker/t/save.dat/full.snapshot", SNAP_FULL);
    tracker_file_commit(fs, 2000, NULL, SNAP_PATCH);

    TEST_ASSERT_EQUAL_UINT64(1000, fs->last_full_mtime);
    TEST_ASSERT_EQUAL_UINT64(2000, fs->last_any_mtime);
    TEST_ASSERT_EQUAL_STRING("sdmc:/switch/savetracker/t/save.dat/full.snapshot", fs->last_full_path);
}

static void test_touch_clears_pending_change(void) {
    FileState *fs = tracker_get_file(&t, "save.dat");
    tracker_file_commit(fs, 1000, "sdmc:/switch/savetracker/t/save.dat/snap.snapshot", SNAP_FULL);
    tracker_file_mark_changed(fs, 200);

    tracker_file_touch(fs, 2000);
    TEST_ASSERT_EQUAL_UINT64(2000, fs->last_seen_mtime);
    TEST_ASSERT_EQUAL_UINT64(0, fs->change_detected_at);
    TEST_ASSERT_EQUAL_UINT64(1000, fs->last_full_mtime);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_no_title_on_first_tick);
    RUN_TEST(test_new_title_detected);
    RUN_TEST(test_same_title_after_detection);
    RUN_TEST(test_same_title_after_interval);
    RUN_TEST(test_title_gone);
    RUN_TEST(test_title_changed);
    RUN_TEST(test_title_reappears_after_gone);
    RUN_TEST(test_set_dir_name);
    RUN_TEST(test_get_file_creates_new);
    RUN_TEST(test_get_file_returns_existing);
    RUN_TEST(test_get_file_grows_dynamically);
    RUN_TEST(test_needs_read_first_time);
    RUN_TEST(test_needs_read_mtime_advanced);
    RUN_TEST(test_no_read_same_mtime);
    RUN_TEST(test_needs_read_mtime_unavailable);
    RUN_TEST(test_delay_not_elapsed_immediately);
    RUN_TEST(test_delay_elapsed_after_wait);
    RUN_TEST(test_mark_changed_only_records_first);
    RUN_TEST(test_commit_updates_state);
    RUN_TEST(test_commit_patch_does_not_update_full_path);
    RUN_TEST(test_touch_clears_pending_change);
    return UNITY_END();
}
