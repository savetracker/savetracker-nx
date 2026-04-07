#ifndef SAVETRACKER_SNAPSHOT_H
#define SAVETRACKER_SNAPSHOT_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef enum {
    SNAP_FULL  = 1,
    SNAP_PATCH = 2,
} SnapKind;

typedef struct {
    uint64_t last_full_mtime; // 0 if no full snapshot yet
    uint64_t last_any_mtime;  // 0 if no snapshot yet
} SnapHistory;

// Returns SNAP_FULL on: no history, clock rollback, or gap > full_interval_secs.
SnapKind snapshot_decide(const SnapHistory *history,
                         uint64_t current_mtime,
                         uint64_t full_interval_secs);

// Builds sd:/switch/savetracker/<title_dir>/<save_filename>/<timestamp>.<snapshot|patch>
// and validates through safe_path_check. Returns false if the path escapes
// the allowed root or the buffer is too small.
bool snapshot_build_path(char *out,
                         size_t out_cap,
                         const char *title_dir,
                         const char *save_filename,
                         const char *timestamp,
                         SnapKind kind);

#endif
