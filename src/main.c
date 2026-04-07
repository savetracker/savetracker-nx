#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <switch.h>

#include "../lib/config.h"
#include "../lib/patch.h"
#include "../lib/snapshot.h"
#include "../lib/timestamp.h"
#include "../lib/tracker.h"
#include "clock.h"
#include "led.h"
#include "power.h"
#include "save_fs.h"
#include "sd_io.h"
#include "title.h"

uint32_t __nx_applet_type = AppletType_None;

#define INNER_HEAP_SIZE 0x80000 // 512 KiB

void __libnx_initheap(void) {
    static char heap[INNER_HEAP_SIZE];
    extern char *fake_heap_start;
    extern char *fake_heap_end;
    fake_heap_start = heap;
    fake_heap_end   = heap + INNER_HEAP_SIZE;
}

// Custom result codes: module 420, description identifies the failure.
// Crash report shows as (2420-NNNN).
#define ST_MODULE 420
#define ST_ERR(n) MAKERESULT(ST_MODULE, n)

#define ST_ERR_SM_INIT        ST_ERR(1)
#define ST_ERR_SETSYS_INIT    ST_ERR(2)
#define ST_ERR_SETSYS_GETFW   ST_ERR(3)
#define ST_ERR_FS_INIT        ST_ERR(4)
#define ST_ERR_SDIO_INIT      ST_ERR(5)
#define ST_ERR_CLOCK_INIT     ST_ERR(6)
#define ST_ERR_POWER_INIT     ST_ERR(7)
#define ST_ERR_TITLE_INIT     ST_ERR(8)
#define ST_ERR_LED_INIT       ST_ERR(9)
#define ST_ERR_SAVEFS_INIT    ST_ERR(10)
#define ST_ERR_PATCH_ALLOC    ST_ERR(11)
#define ST_ERR_BUF_ALLOC      ST_ERR(12)

#define LOG_PATH "sdmc:/switch/savetracker/lastrun.log"

static void stlog(const char *msg) {
    sd_io_log(LOG_PATH, msg);
}

static void stlogf(const char *fmt, uint64_t val) {
    char buf[256];
    snprintf(buf, sizeof(buf), fmt, val);
    sd_io_log(LOG_PATH, buf);
}

#define CHUNK_SIZE      (64ULL * 1024ULL)
#define MAX_CONFIG_SIZE (16ULL * 1024ULL)
#define CONFIG_PATH     "sdmc:/config/savetracker/savetracker.conf"

static Config   g_cfg;
static Tracker  g_tracker;
static Patch   *g_patch   = NULL;
static uint8_t *g_buf_a   = NULL;
static uint8_t *g_buf_b   = NULL;
static uint64_t g_ticks   = 0;

void __appInit(void) {
    svcSleepThread(15000000000ULL);

    if (R_FAILED(smInitialize())) diagAbortWithResult(ST_ERR_SM_INIT);

    if (R_FAILED(setsysInitialize())) diagAbortWithResult(ST_ERR_SETSYS_INIT);
    {
        SetSysFirmwareVersion fw;
        if (R_FAILED(setsysGetFirmwareVersion(&fw))) diagAbortWithResult(ST_ERR_SETSYS_GETFW);
        hosversionSet(MAKEHOSVERSION(fw.major, fw.minor, fw.micro));
        setsysExit();
    }

    if (R_FAILED(fsInitialize())) diagAbortWithResult(ST_ERR_FS_INIT);
    if (!sd_io_init()) diagAbortWithResult(ST_ERR_SDIO_INIT);

    stlog("== savetracker-nx boot ==");

    if (!clock_init()) diagAbortWithResult(ST_ERR_CLOCK_INIT);
    stlog("clock: ok");
    if (!power_init()) diagAbortWithResult(ST_ERR_POWER_INIT);
    stlog("power: ok");
    if (!title_init()) diagAbortWithResult(ST_ERR_TITLE_INIT);
    stlog("title: ok");
    if (!led_init()) diagAbortWithResult(ST_ERR_LED_INIT);
    stlog("led: ok");
    if (!save_fs_init()) diagAbortWithResult(ST_ERR_SAVEFS_INIT);
    stlog("save_fs: ok");
    smExit();
    stlog("init complete");
}

void __appExit(void) {
    save_fs_exit();
    led_exit();
    title_exit();
    power_exit();
    clock_exit();
    sd_io_exit();
    fsExit();
}

static void signal_error(void) {
    led_signal(g_cfg.led_error.color, g_cfg.led_error.duration_secs);
}

static void load_config(void) {
    config_init_defaults(&g_cfg);

    uint8_t *text      = NULL;
    size_t   text_size = 0;
    if (!sd_io_read(CONFIG_PATH, &text, &text_size, MAX_CONFIG_SIZE)) return;

    config_parse(&g_cfg, (const char *)text, text_size);
    sd_io_free(text);
}

static bool write_full_snapshot(const char *filename, const char *path) {
    size_t save_size = 0;
    SaveFileHandle *src = save_fs_open(filename, &save_size);
    if (!src) return false;

    SdWriter *dst = sd_io_open_write(path);
    if (!dst) {
        save_fs_close(src);
        return false;
    }

    size_t offset = 0;
    bool   ok     = true;
    while (offset < save_size) {
        const size_t n = save_fs_read_at(src, offset, g_buf_a, CHUNK_SIZE);
        if (n == 0) { ok = false; break; }
        if (!sd_io_writer_write(dst, g_buf_a, n)) { ok = false; break; }
        offset += n;
    }

    save_fs_close(src);
    if (!ok) {
        sd_io_writer_abort(dst);
        return false;
    }
    return sd_io_writer_close(dst);
}

static bool stream_compare(const char *filename, const char *snapshot_path,
                           size_t save_size, bool *truncated) {
    *truncated = false;

    size_t snap_size = 0;
    SdReader *snap = sd_io_open_read(snapshot_path, &snap_size);
    if (!snap) return true;

    if (save_size < snap_size) {
        *truncated = true;
        sd_io_reader_close(snap);
        return true;
    }

    SaveFileHandle *src = save_fs_open(filename, &save_size);
    if (!src) {
        sd_io_reader_close(snap);
        return true;
    }

    bool differs = (save_size != snap_size);
    size_t offset = 0;
    while (!differs && offset < save_size) {
        const size_t na = save_fs_read_at(src, offset, g_buf_a, CHUNK_SIZE);
        const size_t nb = sd_io_reader_read(snap, g_buf_b, CHUNK_SIZE);
        if (na != nb || memcmp(g_buf_a, g_buf_b, na) != 0) differs = true;
        offset += na;
        if (na == 0) break;
    }

    save_fs_close(src);
    sd_io_reader_close(snap);
    return differs;
}

static bool stream_diff(const char *filename, const char *snapshot_path, size_t save_size) {
    size_t snap_size = 0;
    SdReader *snap = sd_io_open_read(snapshot_path, &snap_size);
    if (!snap) return false;

    SaveFileHandle *src = save_fs_open(filename, &save_size);
    if (!src) {
        sd_io_reader_close(snap);
        return false;
    }

    patch_clear(g_patch);

    size_t       offset  = 0;
    bool         ok      = true;
    const size_t max_len = save_size > snap_size ? save_size : snap_size;

    while (offset < max_len) {
        const size_t na = (offset < save_size) ? save_fs_read_at(src, offset, g_buf_a, CHUNK_SIZE) : 0;
        const size_t nb = (offset < snap_size) ? sd_io_reader_read(snap, g_buf_b, CHUNK_SIZE) : 0;

        if (!patch_diff_chunk(g_patch, g_buf_b, nb, g_buf_a, na, (uint32_t)offset)) {
            ok = false;
            break;
        }

        if (patch_encoded_size(g_patch) > 256 * 1024) {
            ok = false;
            break;
        }

        offset += (na > nb) ? na : nb;
        if (na == 0 && nb == 0) break;
    }

    save_fs_close(src);
    sd_io_reader_close(snap);
    return ok;
}

static bool handle_save_file(void *ctx, const SaveFileMeta *file) {
    (void)ctx;

    FileState *fs = tracker_get_file(&g_tracker, file->name);
    if (!fs) return true;

    if (!tracker_file_needs_read(fs, file->mtime)) return true;

    tracker_file_mark_changed(fs, g_ticks);
    if (!tracker_file_delay_elapsed(fs, &g_cfg, g_ticks)) return true;

    bool truncated = false;
    if (fs->valid && fs->last_full_path[0] != '\0') {
        if (!stream_compare(file->name, fs->last_full_path, file->size, &truncated)) {
            tracker_file_touch(fs, file->mtime);
            return true;
        }
    }

    char ts[TIMESTAMP_BUF_SIZE];
    if (timestamp_format(ts, sizeof(ts), clock_now_secs(), clock_now_millis()) == 0) return true;

    SnapHistory hist = {
        .last_full_mtime = fs->last_full_mtime,
        .last_any_mtime  = fs->last_any_mtime,
    };
    SnapKind kind = snapshot_decide(&hist, file->mtime, g_cfg.full_snapshot_secs);

    if (kind == SNAP_PATCH && (!fs->valid || fs->last_full_path[0] == '\0')) kind = SNAP_FULL;
    if (kind == SNAP_PATCH && truncated) kind = SNAP_FULL;

    char path[512];
    if (!snapshot_build_path(path, sizeof(path), g_tracker.dir_name, file->name, ts, kind)) return true;

    bool ok = false;
    if (kind == SNAP_FULL) {
        ok = write_full_snapshot(file->name, path);
    } else {
        if (stream_diff(file->name, fs->last_full_path, file->size)) {
            uint8_t *encoded      = NULL;
            size_t   encoded_size = 0;
            if (patch_encode(&encoded, &encoded_size, g_patch)) {
                ok = sd_io_write(path, encoded, encoded_size);
                free(encoded);
            }
        } else {
            // Diff failed or patch too large — fall back to full
            if (snapshot_build_path(path, sizeof(path), g_tracker.dir_name, file->name, ts, SNAP_FULL))
                ok = write_full_snapshot(file->name, path);
            kind = SNAP_FULL;
        }
    }

    if (!ok) {
        stlog("snapshot write failed");
        signal_error();
        return true;
    }

    stlog(kind == SNAP_FULL ? "wrote full snapshot" : "wrote patch");
    tracker_file_commit(fs, file->mtime, path, kind);
    return true;
}

static void do_title_check(uint64_t now_ticks) {
    uint64_t   tid     = 0;
    const bool running = title_get_current(&tid);

    if (running) stlogf("title query: running tid=%016lX", (unsigned long)tid);

    TitleChange change = tracker_update_title(&g_tracker, &g_cfg, now_ticks, running, tid);

    switch (change) {
    case TITLE_NEW: {
        stlogf("new title: %016lX", (unsigned long)tid);
        char display[TITLE_NAME_MAX + 1];
        char dir[TITLE_DIR_MAX + 1];
        title_get_name(tid, display, sizeof(display));
        title_build_dir_name(tid, display, dir, sizeof(dir));
        tracker_set_dir_name(&g_tracker, dir);
        stlog(dir);

        if (!save_fs_mount(tid)) {
            stlog("save mount failed");
            tracker_clear(&g_tracker);
            return;
        }
        stlog("save mounted, signaling LED");
        led_signal(g_cfg.led_new_title.color, g_cfg.led_new_title.duration_secs);
        break;
    }
    case TITLE_GONE:
        stlog("title gone");
        save_fs_unmount();
        break;
    case TITLE_NONE:
    case TITLE_SAME:
        break;
    }
}

static void tick(uint64_t now_ticks) {
    if (g_cfg.pause_on_battery) {
        bool charging = false;
        if (!power_is_charging(&charging)) {
            signal_error();
            return;
        }
        if (!charging) return;
    }

    do_title_check(now_ticks);

    if (!g_tracker.active) return;

    save_fs_for_each_file(handle_save_file, NULL);
}

int main(void) {
    load_config();
    tracker_init(&g_tracker);

    g_patch = patch_new();
    g_buf_a = malloc(CHUNK_SIZE);
    g_buf_b = malloc(CHUNK_SIZE);
    if (!g_patch) diagAbortWithResult(ST_ERR_PATCH_ALLOC);
    if (!g_buf_a || !g_buf_b) diagAbortWithResult(ST_ERR_BUF_ALLOC);

    while (1) {
        tick(g_ticks);

        const uint32_t sleep_secs = g_tracker.active ? g_cfg.save_check_secs : g_cfg.idle_secs;
        clock_sleep_secs(sleep_secs);
        g_ticks += sleep_secs;
    }
    return 0;
}
