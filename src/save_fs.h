#ifndef SAVETRACKER_SAVE_FS_H
#define SAVETRACKER_SAVE_FS_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct {
    char     name[256];
    uint64_t size;
    uint64_t mtime; // seconds since epoch, 0 if unavailable
} SaveFileMeta;

// Returning false aborts iteration.
typedef bool (*save_fs_file_cb)(void *ctx, const SaveFileMeta *file);

// Opaque handle for streaming reads from a save file.
typedef struct SaveFileHandle SaveFileHandle;

bool save_fs_init(void);
void save_fs_exit(void);

bool save_fs_mount(uint64_t title_id);
void save_fs_unmount(void);

bool save_fs_for_each_file(save_fs_file_cb cb, void *ctx);

SaveFileHandle *save_fs_open(const char *name, size_t *out_size);
size_t save_fs_read_at(SaveFileHandle *h, size_t offset, uint8_t *buf, size_t cap);
void save_fs_close(SaveFileHandle *h);

#endif
