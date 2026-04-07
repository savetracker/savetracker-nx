#include "snapshot.h"

#include <stdio.h>

#include "safe_path.h"

SnapKind snapshot_decide(const SnapHistory *history,
                         uint64_t current_mtime,
                         uint64_t full_interval_secs) {
    if (!history || history->last_full_mtime == 0) return SNAP_FULL;
    if (current_mtime <= history->last_any_mtime) return SNAP_FULL;
    if ((current_mtime - history->last_full_mtime) > full_interval_secs) return SNAP_FULL;

    return SNAP_PATCH;
}

bool snapshot_build_path(char *out,
                         size_t out_cap,
                         const char *title_dir,
                         const char *save_filename,
                         const char *timestamp,
                         SnapKind kind) {
    if (!out || out_cap == 0 || !title_dir || !save_filename || !timestamp) return false;

    const char *ext = (kind == SNAP_FULL) ? "snapshot" : "patch";

    char raw[512];
    const int n = snprintf(raw,
                           sizeof(raw),
                           "%s/%s/%s/%s.%s",
                           SAFE_PATH_ROOT_SNAPSHOTS,
                           title_dir,
                           save_filename,
                           timestamp,
                           ext);
    if (n < 0 || (size_t)n >= sizeof(raw)) return false;

    return safe_path_check(raw, out, out_cap);
}
