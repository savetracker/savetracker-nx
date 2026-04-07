#ifndef SAVETRACKER_TITLE_H
#define SAVETRACKER_TITLE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// NACP names can be 0x200 bytes; capped for FAT32 readability.
#define TITLE_NAME_MAX 64
#define TITLE_DIR_MAX (16 + 1 + TITLE_NAME_MAX)

bool title_init(void);
void title_exit(void);

// Returns false if no title is running.
bool title_get_current(uint64_t *out_title_id);

// Writes a filesystem-safe display name from NACP. Empty string on failure.
bool title_get_name(uint64_t title_id, char *out, size_t cap);

// Writes "<hex_id>_<name>" or just "<hex_id>" if name is empty.
bool title_build_dir_name(uint64_t title_id,
                          const char *name,
                          char *out,
                          size_t cap);

#endif
