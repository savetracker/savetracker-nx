#include "safe_path.h"

#include <string.h>

#define MAX_SEGMENTS 64

typedef struct {
    const char *ptr;
    size_t      len;
} Segment;

static bool normalize_segments(const char *path, Segment *stack, size_t *count_out) {
    size_t count = 0;
    const char *p = path;

    while (*p) {
        const char *slash = p;
        while (*slash && *slash != '/') slash++;
        const size_t seg_len = (size_t)(slash - p);

        if (seg_len == 0 || (seg_len == 1 && p[0] == '.')) {
            // skip empty or "."
        } else if (seg_len == 2 && p[0] == '.' && p[1] == '.') {
            if (count > 0) count--;
        } else {
            if (count >= MAX_SEGMENTS) return false;
            stack[count].ptr = p;
            stack[count].len = seg_len;
            count++;
        }

        if (*slash == '\0') break;
        p = slash + 1;
    }

    *count_out = count;
    return true;
}

static bool matches_root(const char *normalized, size_t normalized_len, const char *root) {
    const size_t root_len = strlen(root);
    if (normalized_len < root_len) return false;
    if (memcmp(normalized, root, root_len) != 0) return false;

    // Prevent prefix-extension matches like "savetrackerrogue"
    if (normalized_len > root_len && normalized[root_len] != '/') return false;

    return true;
}

static bool has_allowed_prefix(const char *normalized, size_t normalized_len) {
    if (matches_root(normalized, normalized_len, SAFE_PATH_ROOT_SNAPSHOTS)) return true;
    if (matches_root(normalized, normalized_len, SAFE_PATH_ROOT_CONFIG)) return true;
    return false;
}

bool safe_path_check(const char *path, char *out, size_t out_cap) {
    if (!path || !out || out_cap == 0) return false;

    Segment stack[MAX_SEGMENTS];
    size_t  count = 0;
    if (!normalize_segments(path, stack, &count)) return false;

    size_t total = 0;
    for (size_t i = 0; i < count; i++) {
        total += stack[i].len;
        if (i + 1 < count) total += 1;
    }
    if (total + 1 > out_cap) return false;

    size_t pos = 0;
    for (size_t i = 0; i < count; i++) {
        memcpy(out + pos, stack[i].ptr, stack[i].len);
        pos += stack[i].len;
        if (i + 1 < count) out[pos++] = '/';
    }
    out[pos] = '\0';

    return has_allowed_prefix(out, pos);
}
