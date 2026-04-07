#include "save_fs.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <switch.h>

#include "sd_io.h"

#define SAVE_LOG "sdmc:/switch/savetracker/lastrun.log"

static bool         s_mounted = false;
static FsFileSystem s_fs;

struct SaveFileHandle {
    FsFile file;
};

bool save_fs_init(void) {
    return true;
}

void save_fs_exit(void) {
    save_fs_unmount();
}

static bool find_save_uid(uint64_t title_id, AccountUid *out_uid) {
    FsSaveDataInfoReader reader;
    Result rc = fsOpenSaveDataInfoReader(&reader, FsSaveDataSpaceId_User);
    if (R_FAILED(rc)) {
        char msg[128];
        snprintf(msg, sizeof(msg), "save_fs: fsOpenSaveDataInfoReader failed rc=0x%X", rc);
        sd_io_log(SAVE_LOG, msg);
        return false;
    }

    FsSaveDataInfo info;
    s64            total_read = 0;
    bool           found      = false;

    sd_io_log(SAVE_LOG, "save_fs: enumerating save data:");
    while (true) {
        if (R_FAILED(fsSaveDataInfoReaderRead(&reader, &info, 1, &total_read))) break;
        if (total_read == 0) break;

        char msg[256];
        snprintf(msg, sizeof(msg), "  app=%016lX type=%u uid=%016lX%016lX",
                 (unsigned long)info.application_id,
                 (unsigned)info.save_data_type,
                 (unsigned long)info.uid.uid[0],
                 (unsigned long)info.uid.uid[1]);
        sd_io_log(SAVE_LOG, msg);

        if (info.application_id == title_id && info.save_data_type == FsSaveDataType_Account) {
            *out_uid = info.uid;
            found    = true;
            break;
        }
    }

    if (!found) sd_io_log(SAVE_LOG, "save_fs: no matching save found");

    fsSaveDataInfoReaderClose(&reader);
    return found;
}

bool save_fs_mount(uint64_t title_id) {
    save_fs_unmount();

    AccountUid uid = {0};
    if (!find_save_uid(title_id, &uid)) {
        sd_io_log(SAVE_LOG, "save_fs: no Account-type save found for title");
        return false;
    }

    char msg[128];
    snprintf(msg, sizeof(msg), "save_fs: found uid %016lX%016lX",
             (unsigned long)uid.uid[0], (unsigned long)uid.uid[1]);
    sd_io_log(SAVE_LOG, msg);

    // Use read-only open — the game holds an exclusive lock on its save via
    // cmd 51 (OpenSaveDataFileSystem). Cmd 53 (OpenReadOnlySaveDataFileSystem)
    // allows concurrent read access.
    Result rc = fsOpen_SaveDataReadOnly(&s_fs, (u64)title_id, uid);
    if (R_FAILED(rc)) {
        snprintf(msg, sizeof(msg), "save_fs: fsOpen_SaveDataReadOnly failed rc=0x%X", rc);
        sd_io_log(SAVE_LOG, msg);
        return false;
    }

    sd_io_log(SAVE_LOG, "save_fs: mounted");
    s_mounted = true;
    return true;
}

void save_fs_unmount(void) {
    if (!s_mounted) return;

    fsFsClose(&s_fs);
    s_mounted = false;
}

bool save_fs_for_each_file(save_fs_file_cb cb, void *ctx) {
    if (!s_mounted || !cb) return false;

    FsDir dir;
    if (R_FAILED(fsFsOpenDirectory(&s_fs, "/", FsDirOpenMode_ReadFiles, &dir))) return false;

    bool ok = true;
    while (true) {
        FsDirectoryEntry entry;
        s64              read = 0;
        if (R_FAILED(fsDirRead(&dir, &read, 1, &entry))) {
            ok = false;
            break;
        }
        if (read == 0) break;
        if (entry.type != FsDirEntryType_File) continue;

        char path[260];
        snprintf(path, sizeof(path), "/%s", entry.name);

        FsTimeStampRaw ts  = {0};
        uint64_t       mod = 0;
        if (R_SUCCEEDED(fsFsGetFileTimeStampRaw(&s_fs, path, &ts))) mod = ts.modified;

        SaveFileMeta meta = {0};
        snprintf(meta.name, sizeof(meta.name), "%s", entry.name);
        meta.size  = (uint64_t)entry.file_size;
        meta.mtime = mod;

        if (!cb(ctx, &meta)) break;
    }

    fsDirClose(&dir);
    return ok;
}

SaveFileHandle *save_fs_open(const char *name, size_t *out_size) {
    if (!s_mounted || !name || !out_size) return NULL;

    char path[260];
    snprintf(path, sizeof(path), "/%s", name);

    SaveFileHandle *h = malloc(sizeof(SaveFileHandle));
    if (!h) return NULL;

    if (R_FAILED(fsFsOpenFile(&s_fs, path, FsOpenMode_Read, &h->file))) {
        free(h);
        return NULL;
    }

    s64 size = 0;
    if (R_FAILED(fsFileGetSize(&h->file, &size)) || size < 0) {
        fsFileClose(&h->file);
        free(h);
        return NULL;
    }

    *out_size = (size_t)size;
    return h;
}

size_t save_fs_read_at(SaveFileHandle *h, size_t offset, uint8_t *buf, size_t cap) {
    if (!h || !buf || cap == 0) return 0;

    u64 out = 0;
    if (R_FAILED(fsFileRead(&h->file, (s64)offset, buf, (u64)cap, FsReadOption_None, &out))) return 0;

    return (size_t)out;
}

void save_fs_close(SaveFileHandle *h) {
    if (!h) return;
    fsFileClose(&h->file);
    free(h);
}
