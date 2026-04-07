#ifndef SAVETRACKER_SAFE_PATH_H
#define SAVETRACKER_SAFE_PATH_H

#include <stdbool.h>
#include <stddef.h>

#define SAFE_PATH_ROOT_SNAPSHOTS "sdmc:/switch/savetracker"
#define SAFE_PATH_ROOT_CONFIG    "sdmc:/config/savetracker"

// Normalizes a path (resolves ./.., collapses duplicate slashes) and verifies
// it falls under one of the allowed roots. Returns false if the normalized
// path escapes or the buffer is too small.
bool safe_path_check(const char *path, char *out, size_t out_cap);

#endif
