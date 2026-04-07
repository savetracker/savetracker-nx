#include "patch.h"

#include <stdlib.h>
#include <string.h>

// u16 length field caps each region
#define MAX_REGION_LEN 0xFFFF

typedef struct {
    uint32_t offset;
    uint8_t *data;
    size_t   data_len;
} PatchRegion;

struct Patch {
    PatchRegion *regions;
    size_t       regions_len;
    size_t       regions_cap;
};

static bool regions_grow(Patch *p) {
    const size_t new_cap     = p->regions_cap ? p->regions_cap * 2 : 4;
    PatchRegion *new_regions = realloc(p->regions, new_cap * sizeof(PatchRegion));
    if (!new_regions) return false;

    p->regions     = new_regions;
    p->regions_cap = new_cap;
    return true;
}

static bool regions_push(Patch *p, uint32_t offset, const uint8_t *data, size_t len) {
    if (p->regions_len == p->regions_cap && !regions_grow(p)) return false;

    uint8_t *buf = malloc(len);
    if (!buf) return false;

    memcpy(buf, data, len);
    p->regions[p->regions_len].offset   = offset;
    p->regions[p->regions_len].data     = buf;
    p->regions[p->regions_len].data_len = len;
    p->regions_len++;
    return true;
}

Patch *patch_new(void) {
    Patch *p = malloc(sizeof(Patch));
    if (!p) return NULL;

    p->regions     = NULL;
    p->regions_len = 0;
    p->regions_cap = 0;
    return p;
}

void patch_free(Patch *p) {
    if (!p) return;

    for (size_t i = 0; i < p->regions_len; i++) free(p->regions[i].data);
    free(p->regions);
    free(p);
}

void patch_clear(Patch *p) {
    if (!p) return;

    for (size_t i = 0; i < p->regions_len; i++) free(p->regions[i].data);
    p->regions_len = 0;
}

bool patch_is_empty(const Patch *p) {
    return !p || p->regions_len == 0;
}

size_t patch_region_count(const Patch *p) {
    return p ? p->regions_len : 0;
}

size_t patch_encoded_size(const Patch *p) {
    if (!p) return 0;

    size_t total = 0;
    for (size_t i = 0; i < p->regions_len; i++) total += 4 + 2 + p->regions[i].data_len;
    return total;
}

bool patch_diff_chunk(Patch *out,
                      const uint8_t *old_data, size_t old_len,
                      const uint8_t *new_data, size_t new_len,
                      uint32_t base_offset) {
    const size_t min_len = old_len < new_len ? old_len : new_len;

    size_t i = 0;
    while (i < min_len) {
        if (old_data[i] == new_data[i]) {
            i++;
            continue;
        }

        const size_t start = i;
        while (i < min_len && old_data[i] != new_data[i] && (i - start) < MAX_REGION_LEN) i++;

        if (!regions_push(out, base_offset + (uint32_t)start, &new_data[start], i - start)) return false;
    }

    if (new_len > min_len) {
        size_t pos = min_len;
        while (pos < new_len) {
            size_t chunk = new_len - pos;
            if (chunk > MAX_REGION_LEN) chunk = MAX_REGION_LEN;

            if (!regions_push(out, base_offset + (uint32_t)pos, &new_data[pos], chunk)) return false;
            pos += chunk;
        }
    }

    return true;
}

bool patch_diff(Patch *out,
                const uint8_t *old_data, size_t old_len,
                const uint8_t *new_data, size_t new_len) {
    return patch_diff_chunk(out, old_data, old_len, new_data, new_len, 0);
}

bool patch_apply(uint8_t **out, size_t *out_len,
                 const uint8_t *base, size_t base_len,
                 const Patch *p) {
    size_t max_end = base_len;
    for (size_t i = 0; i < p->regions_len; i++) {
        const size_t end = (size_t)p->regions[i].offset + p->regions[i].data_len;
        if (end > max_end) max_end = end;
    }

    uint8_t *buf = malloc(max_end == 0 ? 1 : max_end);
    if (!buf) return false;

    if (base_len > 0) memcpy(buf, base, base_len);
    if (max_end > base_len) memset(buf + base_len, 0, max_end - base_len);

    for (size_t i = 0; i < p->regions_len; i++) {
        memcpy(buf + p->regions[i].offset, p->regions[i].data, p->regions[i].data_len);
    }

    *out     = buf;
    *out_len = max_end;
    return true;
}

// Wire format: repeated [offset: u32 LE][length: u16 LE][data: length bytes]
bool patch_encode(uint8_t **out, size_t *out_len, const Patch *p) {
    const size_t total = patch_encoded_size(p);
    uint8_t     *buf   = malloc(total == 0 ? 1 : total);
    if (!buf) return false;

    size_t pos = 0;
    for (size_t i = 0; i < p->regions_len; i++) {
        const uint32_t off = p->regions[i].offset;
        const uint16_t len = (uint16_t)p->regions[i].data_len;
        buf[pos++]         = (uint8_t)(off & 0xFF);
        buf[pos++]         = (uint8_t)((off >> 8) & 0xFF);
        buf[pos++]         = (uint8_t)((off >> 16) & 0xFF);
        buf[pos++]         = (uint8_t)((off >> 24) & 0xFF);
        buf[pos++]         = (uint8_t)(len & 0xFF);
        buf[pos++]         = (uint8_t)((len >> 8) & 0xFF);
        memcpy(buf + pos, p->regions[i].data, p->regions[i].data_len);
        pos += p->regions[i].data_len;
    }

    *out     = buf;
    *out_len = total;
    return true;
}

bool patch_decode(Patch *out, const uint8_t *data, size_t data_len) {
    size_t pos = 0;

    while (pos < data_len) {
        if (pos + 6 > data_len) {
            patch_clear(out);
            return false;
        }

        const uint32_t offset = (uint32_t)data[pos]
                                | ((uint32_t)data[pos + 1] << 8)
                                | ((uint32_t)data[pos + 2] << 16)
                                | ((uint32_t)data[pos + 3] << 24);
        pos += 4;

        const size_t length = (size_t)data[pos] | ((size_t)data[pos + 1] << 8);
        pos += 2;

        if (pos + length > data_len) {
            patch_clear(out);
            return false;
        }

        if (!regions_push(out, offset, &data[pos], length)) {
            patch_clear(out);
            return false;
        }
        pos += length;
    }

    return true;
}

bool patch_should_patch(const uint8_t *old_data, size_t old_len,
                        const uint8_t *new_data, size_t new_len) {
    if (old_len == new_len && memcmp(old_data, new_data, old_len) == 0) return false;
    if (new_len < old_len) return false; // truncation needs a full snapshot
    return true;
}
