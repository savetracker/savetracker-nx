#include <stdlib.h>
#include <string.h>

#include "patch.h"
#include "unity/unity.h"

static Patch *p;

void setUp(void) {
    p = patch_new();
    TEST_ASSERT_NOT_NULL(p);
}

void tearDown(void) {
    patch_free(p);
    p = NULL;
}

static void test_identical_files_produce_empty_patch(void) {
    const uint8_t data[] = "hello world";
    TEST_ASSERT_TRUE(patch_diff(p, data, sizeof(data) - 1, data, sizeof(data) - 1));
    TEST_ASSERT_TRUE(patch_is_empty(p));
}

static void test_single_byte_change(void) {
    const uint8_t old_data[] = "hello";
    const uint8_t new_data[] = "hxllo";
    TEST_ASSERT_TRUE(patch_diff(p, old_data, 5, new_data, 5));
    TEST_ASSERT_EQUAL_UINT32(1, patch_region_count(p));

    uint8_t *result     = NULL;
    size_t   result_len = 0;
    TEST_ASSERT_TRUE(patch_apply(&result, &result_len, old_data, 5, p));
    TEST_ASSERT_EQUAL_MEMORY(new_data, result, 5);
    free(result);
}

static void test_multiple_changed_regions(void) {
    const uint8_t old_data[] = "aabbcc";
    const uint8_t new_data[] = "aXbbYc";
    TEST_ASSERT_TRUE(patch_diff(p, old_data, 6, new_data, 6));
    TEST_ASSERT_EQUAL_UINT32(2, patch_region_count(p));

    uint8_t *result     = NULL;
    size_t   result_len = 0;
    TEST_ASSERT_TRUE(patch_apply(&result, &result_len, old_data, 6, p));
    TEST_ASSERT_EQUAL_MEMORY(new_data, result, 6);
    free(result);
}

static void test_appended_data(void) {
    const uint8_t old_data[] = "abc";
    const uint8_t new_data[] = "abcdef";
    TEST_ASSERT_TRUE(patch_diff(p, old_data, 3, new_data, 6));
    TEST_ASSERT_EQUAL_UINT32(1, patch_region_count(p));

    uint8_t *result     = NULL;
    size_t   result_len = 0;
    TEST_ASSERT_TRUE(patch_apply(&result, &result_len, old_data, 3, p));
    TEST_ASSERT_EQUAL_UINT32(6, result_len);
    TEST_ASSERT_EQUAL_MEMORY(new_data, result, 6);
    free(result);
}

static void test_apply_produces_new_version(void) {
    const uint8_t old_data[] = "hello world";
    const uint8_t new_data[] = "hello WORLD";
    TEST_ASSERT_TRUE(patch_diff(p, old_data, 11, new_data, 11));

    uint8_t *result     = NULL;
    size_t   result_len = 0;
    TEST_ASSERT_TRUE(patch_apply(&result, &result_len, old_data, 11, p));
    TEST_ASSERT_EQUAL_UINT32(11, result_len);
    TEST_ASSERT_EQUAL_MEMORY(new_data, result, 11);
    free(result);
}

static void test_apply_with_appended_data(void) {
    const uint8_t old_data[] = "abc";
    const uint8_t new_data[] = "abcXYZ";
    TEST_ASSERT_TRUE(patch_diff(p, old_data, 3, new_data, 6));

    uint8_t *result     = NULL;
    size_t   result_len = 0;
    TEST_ASSERT_TRUE(patch_apply(&result, &result_len, old_data, 3, p));
    TEST_ASSERT_EQUAL_UINT32(6, result_len);
    TEST_ASSERT_EQUAL_MEMORY(new_data, result, 6);
    free(result);
}

static void test_encode_decode_roundtrip(void) {
    const uint8_t old_data[] = "the quick brown fox";
    const uint8_t new_data[] = "the slow  brown cat";
    TEST_ASSERT_TRUE(patch_diff(p, old_data, 19, new_data, 19));

    uint8_t *encoded     = NULL;
    size_t   encoded_len = 0;
    TEST_ASSERT_TRUE(patch_encode(&encoded, &encoded_len, p));
    TEST_ASSERT_EQUAL_UINT32(patch_encoded_size(p), encoded_len);

    Patch *decoded = patch_new();
    TEST_ASSERT_NOT_NULL(decoded);
    TEST_ASSERT_TRUE(patch_decode(decoded, encoded, encoded_len));
    TEST_ASSERT_EQUAL_UINT32(patch_region_count(p), patch_region_count(decoded));

    // Verify the decoded patch produces the same result as the original
    uint8_t *r1 = NULL, *r2 = NULL;
    size_t   l1 = 0,     l2 = 0;
    TEST_ASSERT_TRUE(patch_apply(&r1, &l1, old_data, 19, p));
    TEST_ASSERT_TRUE(patch_apply(&r2, &l2, old_data, 19, decoded));
    TEST_ASSERT_EQUAL_UINT32(l1, l2);
    TEST_ASSERT_EQUAL_MEMORY(r1, r2, l1);

    free(r1);
    free(r2);
    patch_free(decoded);
    free(encoded);
}

static void test_decode_empty(void) {
    TEST_ASSERT_TRUE(patch_decode(p, NULL, 0));
    TEST_ASSERT_TRUE(patch_is_empty(p));
}

static void test_decode_truncated_header_returns_false(void) {
    const uint8_t data[] = {0x00, 0x00};
    TEST_ASSERT_FALSE(patch_decode(p, data, 2));
}

static void test_decode_truncated_data_returns_false(void) {
    const uint8_t data[] = {
        0x00, 0x00, 0x00, 0x00,
        0x0A, 0x00,
        0x00, 0x00,
    };
    TEST_ASSERT_FALSE(patch_decode(p, data, sizeof(data)));
}

static void test_full_roundtrip_diff_encode_decode_apply(void) {
    const uint8_t old_data[] = "save: level=5 gold=100 hp=80";
    const uint8_t new_data[] = "save: level=6 gold=250 hp=80";
    const size_t  len        = 28;

    TEST_ASSERT_TRUE(patch_diff(p, old_data, len, new_data, len));

    uint8_t *encoded     = NULL;
    size_t   encoded_len = 0;
    TEST_ASSERT_TRUE(patch_encode(&encoded, &encoded_len, p));

    Patch *decoded = patch_new();
    TEST_ASSERT_NOT_NULL(decoded);
    TEST_ASSERT_TRUE(patch_decode(decoded, encoded, encoded_len));

    uint8_t *result     = NULL;
    size_t   result_len = 0;
    TEST_ASSERT_TRUE(patch_apply(&result, &result_len, old_data, len, decoded));
    TEST_ASSERT_EQUAL_UINT32(len, result_len);
    TEST_ASSERT_EQUAL_MEMORY(new_data, result, len);

    free(result);
    patch_free(decoded);
    free(encoded);
}

static void test_should_patch_identical(void) {
    const uint8_t data[] = "same";
    TEST_ASSERT_FALSE(patch_should_patch(data, 4, data, 4));
}

static void test_should_patch_changed(void) {
    TEST_ASSERT_TRUE(patch_should_patch((const uint8_t *)"old", 3, (const uint8_t *)"new", 3));
}

static void test_should_patch_truncation_returns_false(void) {
    TEST_ASSERT_FALSE(patch_should_patch((const uint8_t *)"longer", 6, (const uint8_t *)"short", 5));
}

static void test_should_patch_appended(void) {
    TEST_ASSERT_TRUE(patch_should_patch((const uint8_t *)"abc", 3, (const uint8_t *)"abcdef", 6));
}

static void test_contiguous_changes_merge_into_one_region(void) {
    const uint8_t old_data[] = "abcd";
    const uint8_t new_data[] = "XXXX";
    TEST_ASSERT_TRUE(patch_diff(p, old_data, 4, new_data, 4));
    TEST_ASSERT_EQUAL_UINT32(1, patch_region_count(p));

    uint8_t *result     = NULL;
    size_t   result_len = 0;
    TEST_ASSERT_TRUE(patch_apply(&result, &result_len, old_data, 4, p));
    TEST_ASSERT_EQUAL_MEMORY(new_data, result, 4);
    free(result);
}

static void test_diff_chunk_with_offset(void) {
    const uint8_t old_data[] = "aabbccdd";
    const uint8_t new_data[] = "aaXXccYY";

    TEST_ASSERT_TRUE(patch_diff_chunk(p, old_data, 4, new_data, 4, 0));
    TEST_ASSERT_TRUE(patch_diff_chunk(p, old_data + 4, 4, new_data + 4, 4, 4));
    TEST_ASSERT_EQUAL_UINT32(2, patch_region_count(p));

    uint8_t *result     = NULL;
    size_t   result_len = 0;
    TEST_ASSERT_TRUE(patch_apply(&result, &result_len, old_data, 8, p));
    TEST_ASSERT_EQUAL_UINT32(8, result_len);
    TEST_ASSERT_EQUAL_MEMORY(new_data, result, 8);
    free(result);
}

static void test_diff_chunk_appended_in_second_chunk(void) {
    const uint8_t old_data[] = "abcd";
    const uint8_t new_data[] = "abcdEFGH";

    TEST_ASSERT_TRUE(patch_diff_chunk(p, old_data, 4, new_data, 4, 0));
    TEST_ASSERT_TRUE(patch_diff_chunk(p, NULL, 0, new_data + 4, 4, 4));
    TEST_ASSERT_EQUAL_UINT32(1, patch_region_count(p));

    uint8_t *result     = NULL;
    size_t   result_len = 0;
    TEST_ASSERT_TRUE(patch_apply(&result, &result_len, old_data, 4, p));
    TEST_ASSERT_EQUAL_UINT32(8, result_len);
    TEST_ASSERT_EQUAL_MEMORY(new_data, result, 8);
    free(result);
}

static void test_diff_chunk_mutation_across_boundary(void) {
    // Simulate the real 64KB chunk size. Build two buffers that are 2.5 chunks
    // long, with a contiguous mutation that spans the boundary between chunk 1
    // and chunk 2.
    const size_t chunk   = 64 * 1024;
    const size_t total   = chunk * 2 + chunk / 2;
    uint8_t *old_data    = malloc(total);
    uint8_t *new_data    = malloc(total);
    TEST_ASSERT_NOT_NULL(old_data);
    TEST_ASSERT_NOT_NULL(new_data);

    memset(old_data, 'A', total);
    memcpy(new_data, old_data, total);

    // Mutate 16 bytes straddling the first chunk boundary (last 8 of chunk 0,
    // first 8 of chunk 1).
    const size_t mut_start = chunk - 8;
    memset(new_data + mut_start, 'Z', 16);

    // Diff in chunks, same way main.c does
    for (size_t off = 0; off < total; off += chunk) {
        size_t old_len = total - off;
        if (old_len > chunk) old_len = chunk;
        size_t new_len = old_len;

        TEST_ASSERT_TRUE(patch_diff_chunk(p,
            old_data + off, old_len,
            new_data + off, new_len,
            (uint32_t)off));
    }

    // The mutation spans a chunk boundary so it will produce 2 regions
    // (one per chunk), but both must reconstruct correctly.
    TEST_ASSERT_TRUE(patch_region_count(p) >= 1);

    uint8_t *result     = NULL;
    size_t   result_len = 0;
    TEST_ASSERT_TRUE(patch_apply(&result, &result_len, old_data, total, p));
    TEST_ASSERT_EQUAL_UINT32(total, result_len);
    TEST_ASSERT_EQUAL_MEMORY(new_data, result, total);

    // Verify the encode→decode→apply roundtrip too
    uint8_t *encoded     = NULL;
    size_t   encoded_len = 0;
    TEST_ASSERT_TRUE(patch_encode(&encoded, &encoded_len, p));

    Patch *decoded = patch_new();
    TEST_ASSERT_NOT_NULL(decoded);
    TEST_ASSERT_TRUE(patch_decode(decoded, encoded, encoded_len));

    uint8_t *result2     = NULL;
    size_t   result2_len = 0;
    TEST_ASSERT_TRUE(patch_apply(&result2, &result2_len, old_data, total, decoded));
    TEST_ASSERT_EQUAL_UINT32(total, result2_len);
    TEST_ASSERT_EQUAL_MEMORY(new_data, result2, total);

    free(result2);
    patch_free(decoded);
    free(encoded);
    free(result);
    free(new_data);
    free(old_data);
}

static void test_clear_reuses_allocation(void) {
    const uint8_t old_data[] = "hello";
    const uint8_t new_data[] = "hxllo";
    TEST_ASSERT_TRUE(patch_diff(p, old_data, 5, new_data, 5));
    TEST_ASSERT_EQUAL_UINT32(1, patch_region_count(p));

    patch_clear(p);
    TEST_ASSERT_TRUE(patch_is_empty(p));

    TEST_ASSERT_TRUE(patch_diff(p, old_data, 5, new_data, 5));
    TEST_ASSERT_EQUAL_UINT32(1, patch_region_count(p));
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_identical_files_produce_empty_patch);
    RUN_TEST(test_single_byte_change);
    RUN_TEST(test_multiple_changed_regions);
    RUN_TEST(test_appended_data);
    RUN_TEST(test_apply_produces_new_version);
    RUN_TEST(test_apply_with_appended_data);
    RUN_TEST(test_encode_decode_roundtrip);
    RUN_TEST(test_decode_empty);
    RUN_TEST(test_decode_truncated_header_returns_false);
    RUN_TEST(test_decode_truncated_data_returns_false);
    RUN_TEST(test_full_roundtrip_diff_encode_decode_apply);
    RUN_TEST(test_should_patch_identical);
    RUN_TEST(test_should_patch_changed);
    RUN_TEST(test_should_patch_truncation_returns_false);
    RUN_TEST(test_should_patch_appended);
    RUN_TEST(test_contiguous_changes_merge_into_one_region);
    RUN_TEST(test_diff_chunk_with_offset);
    RUN_TEST(test_diff_chunk_appended_in_second_chunk);
    RUN_TEST(test_diff_chunk_mutation_across_boundary);
    RUN_TEST(test_clear_reuses_allocation);
    return UNITY_END();
}
