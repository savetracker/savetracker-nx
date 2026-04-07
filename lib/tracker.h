#ifndef SAVETRACKER_TRACKER_H
#define SAVETRACKER_TRACKER_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "config.h"
#include "snapshot.h"

#define TRACKER_DIR_MAX 82

typedef struct {
    char     name[256];
    uint64_t last_seen_mtime;
    uint64_t last_full_mtime;
    uint64_t last_any_mtime;
    char     last_full_path[512];
    uint64_t change_detected_at;
    bool     valid;
} FileState;

typedef struct {
    uint64_t   title_id;
    char       dir_name[TRACKER_DIR_MAX + 1];
    FileState *files;
    size_t     file_count;
    size_t     file_cap;
    bool       active;
    uint64_t   last_title_check;
    bool       first_tick;
} Tracker;

typedef enum {
    TITLE_NONE,
    TITLE_SAME,
    TITLE_NEW,
    TITLE_GONE,
} TitleChange;

void tracker_init(Tracker *t);
void tracker_clear(Tracker *t);

TitleChange tracker_update_title(Tracker *t, const Config *cfg,
                                 uint64_t now_ticks,
                                 bool title_running, uint64_t title_id);

void tracker_set_dir_name(Tracker *t, const char *dir_name);

FileState *tracker_get_file(Tracker *t, const char *filename);

bool tracker_file_needs_read(const FileState *fs, uint64_t current_mtime);

bool tracker_file_delay_elapsed(const FileState *fs, const Config *cfg,
                                uint64_t now_ticks);

void tracker_file_mark_changed(FileState *fs, uint64_t now_ticks);

void tracker_file_commit(FileState *fs, uint64_t mtime,
                         const char *full_path, SnapKind kind);

void tracker_file_touch(FileState *fs, uint64_t mtime);

#endif
