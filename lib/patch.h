#ifndef SAVETRACKER_PATCH_H
#define SAVETRACKER_PATCH_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// Opaque. Allocate with patch_new(), free with patch_free().
// Byte-compatible with savetracker-core's Rust patch module.
typedef struct Patch Patch;

Patch *patch_new(void);
void   patch_free(Patch *p);

// Clears regions without freeing the Patch itself.
void   patch_clear(Patch *p);

bool   patch_is_empty(const Patch *p);
size_t patch_region_count(const Patch *p);
size_t patch_encoded_size(const Patch *p);

// Returns false on allocation failure.
bool patch_diff(Patch *out,
                const uint8_t *old_data, size_t old_len,
                const uint8_t *new_data, size_t new_len);

// Like patch_diff but appends regions at base_offset. Call repeatedly with
// sequential chunks to diff files without loading them entirely into memory.
bool patch_diff_chunk(Patch *out,
                      const uint8_t *old_data, size_t old_len,
                      const uint8_t *new_data, size_t new_len,
                      uint32_t base_offset);

// Caller owns *out and must free() it.
bool patch_apply(uint8_t **out, size_t *out_len,
                 const uint8_t *base, size_t base_len,
                 const Patch *p);

// Caller owns *out and must free() it.
bool patch_encode(uint8_t **out, size_t *out_len, const Patch *p);

// Returns false on malformed input.
bool patch_decode(Patch *out, const uint8_t *data, size_t data_len);

// Returns false for identical files or truncation (use a full snapshot instead).
bool patch_should_patch(const uint8_t *old_data, size_t old_len,
                        const uint8_t *new_data, size_t new_len);

#endif
