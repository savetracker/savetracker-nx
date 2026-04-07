#include <string.h>

#include "timestamp.h"
#include "unity/unity.h"

static char buf[TIMESTAMP_BUF_SIZE];

void setUp(void) {
    memset(buf, 0, sizeof(buf));
}

void tearDown(void) {}

static void test_unix_epoch(void) {
    TEST_ASSERT_EQUAL_UINT32(19, timestamp_format(buf, sizeof(buf), 0, 0));
    TEST_ASSERT_EQUAL_STRING("19700101_000000_000", buf);
}

static void test_new_year_2020(void) {
    // 2020-01-01 00:00:00 UTC = 1577836800
    TEST_ASSERT_EQUAL_UINT32(19, timestamp_format(buf, sizeof(buf), 1577836800ULL, 0));
    TEST_ASSERT_EQUAL_STRING("20200101_000000_000", buf);
}

static void test_new_year_2021(void) {
    // 2021-01-01 00:00:00 UTC = 1609459200
    TEST_ASSERT_EQUAL_UINT32(19, timestamp_format(buf, sizeof(buf), 1609459200ULL, 0));
    TEST_ASSERT_EQUAL_STRING("20210101_000000_000", buf);
}

static void test_millis_formatting(void) {
    TEST_ASSERT_EQUAL_UINT32(19, timestamp_format(buf, sizeof(buf), 1577836800ULL, 42));
    TEST_ASSERT_EQUAL_STRING("20200101_000000_042", buf);
}

static void test_max_millis(void) {
    TEST_ASSERT_EQUAL_UINT32(19, timestamp_format(buf, sizeof(buf), 1577836800ULL, 999));
    TEST_ASSERT_EQUAL_STRING("20200101_000000_999", buf);
}

static void test_millis_over_1000_clamped(void) {
    TEST_ASSERT_EQUAL_UINT32(19, timestamp_format(buf, sizeof(buf), 1577836800ULL, 1234));
    TEST_ASSERT_EQUAL_STRING("20200101_000000_999", buf);
}

static void test_time_of_day(void) {
    // 2020-06-15 13:45:30 UTC = 1592228730
    TEST_ASSERT_EQUAL_UINT32(19, timestamp_format(buf, sizeof(buf), 1592228730ULL, 123));
    TEST_ASSERT_EQUAL_STRING("20200615_134530_123", buf);
}

static void test_zero_pads_single_digits(void) {
    // 2020-01-02 03:04:05 UTC = 1577934245
    TEST_ASSERT_EQUAL_UINT32(19, timestamp_format(buf, sizeof(buf), 1577934245ULL, 6));
    TEST_ASSERT_EQUAL_STRING("20200102_030405_006", buf);
}

static void test_buffer_too_small(void) {
    char small[10];
    TEST_ASSERT_EQUAL_UINT32(0, timestamp_format(small, sizeof(small), 1577836800ULL, 0));
}

static void test_null_buffer(void) {
    TEST_ASSERT_EQUAL_UINT32(0, timestamp_format(NULL, 20, 1577836800ULL, 0));
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_unix_epoch);
    RUN_TEST(test_new_year_2020);
    RUN_TEST(test_new_year_2021);
    RUN_TEST(test_millis_formatting);
    RUN_TEST(test_max_millis);
    RUN_TEST(test_millis_over_1000_clamped);
    RUN_TEST(test_time_of_day);
    RUN_TEST(test_zero_pads_single_digits);
    RUN_TEST(test_buffer_too_small);
    RUN_TEST(test_null_buffer);
    return UNITY_END();
}
