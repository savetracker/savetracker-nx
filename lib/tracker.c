#include "tracker.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void tracker_init(Tracker *t) {
    memset(t, 0, sizeof(*t));
    t->first_tick = true;
}

void tracker_clear(Tracker *t) {
    free(t->files);

    const bool first = t->first_tick;
    memset(t, 0, sizeof(*t));
    t->first_tick = first;
}

TitleChange tracker_update_title(Tracker *t, const Config *cfg,
                                 uint64_t now_ticks,
                                 bool title_running, uint64_t title_id) {
    if (!t->first_tick && (now_ticks - t->last_title_check) < cfg->title_check_secs)
        return t->active ? TITLE_SAME : TITLE_NONE;

    t->first_tick       = false;
    t->last_title_check = now_ticks;

    if (!title_running) {
        if (t->active) {
            tracker_clear(t);
            return TITLE_GONE;
        }
        return TITLE_NONE;
    }

    if (t->active && t->title_id == title_id) return TITLE_SAME;

    tracker_clear(t);
    t->title_id = title_id;
    t->active   = true;
    return TITLE_NEW;
}

void tracker_set_dir_name(Tracker *t, const char *dir_name) {
    if (!dir_name) return;
    snprintf(t->dir_name, sizeof(t->dir_name), "%s", dir_name);
}

FileState *tracker_get_file(Tracker *t, const char *filename) {
    for (size_t i = 0; i < t->file_count; i++) {
        if (strcmp(t->files[i].name, filename) == 0) return &t->files[i];
    }

    if (t->file_count == t->file_cap) {
        const size_t new_cap = t->file_cap ? t->file_cap * 2 : 4;
        FileState *grown = realloc(t->files, new_cap * sizeof(FileState));
        if (!grown) return NULL;
        t->files    = grown;
        t->file_cap = new_cap;
    }

    FileState *fs = &t->files[t->file_count++];
    memset(fs, 0, sizeof(*fs));
    snprintf(fs->name, sizeof(fs->name), "%s", filename);
    return fs;
}

bool tracker_file_needs_read(const FileState *fs, uint64_t current_mtime) {
    if (!fs->valid) return true;
    if (current_mtime == 0) return true;
    return current_mtime > fs->last_seen_mtime;
}

bool tracker_file_delay_elapsed(const FileState *fs, const Config *cfg,
                                uint64_t now_ticks) {
    if (fs->change_detected_at == 0) return false;
    return (now_ticks - fs->change_detected_at) >= cfg->write_delay_secs;
}

void tracker_file_mark_changed(FileState *fs, uint64_t now_ticks) {
    if (fs->change_detected_at == 0) fs->change_detected_at = now_ticks;
}

void tracker_file_commit(FileState *fs, uint64_t mtime,
                         const char *full_path, SnapKind kind) {
    fs->last_seen_mtime    = mtime;
    fs->last_any_mtime     = mtime;
    fs->change_detected_at = 0;
    if (kind == SNAP_FULL && full_path) {
        snprintf(fs->last_full_path, sizeof(fs->last_full_path), "%s", full_path);
        fs->last_full_mtime = mtime;
    }
    fs->valid = true;
}

void tracker_file_touch(FileState *fs, uint64_t mtime) {
    fs->last_seen_mtime    = mtime;
    fs->change_detected_at = 0;
}
