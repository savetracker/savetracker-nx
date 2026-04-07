// Only file allowed to call filesystem write functions (lint-enforced).

#include "sd_io.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include <switch.h>

#include "safe_path.h"

struct SdReader {
    FILE *fp;
};

struct SdWriter {
    FILE *fp;
    char  final_path[512];
    char  tmp_path[520];
};

bool sd_io_init(void) {
    return R_SUCCEEDED(fsdevMountSdmc());
}

void sd_io_exit(void) {
    fsdevUnmountAll();
}

static bool ensure_dirs(const char *path) {
    char   tmp[512];
    size_t len = strlen(path);
    if (len >= sizeof(tmp)) return false;

    memcpy(tmp, path, len + 1);

    char *p = strchr(tmp, '/');
    if (!p) return true;

    for (p = p + 1; *p; p++) {
        if (*p != '/') continue;

        *p = '\0';
        if (mkdir(tmp, 0755) != 0 && errno != EEXIST) {
            *p = '/';
            return false;
        }
        *p = '/';
    }

    if (mkdir(tmp, 0755) != 0 && errno != EEXIST) return false;

    return true;
}

static char s_last_ensured_dir[512] = {0};

static bool ensure_parent_dirs(const char *safe_path) {
    char dir[512];
    const size_t len = strlen(safe_path);
    if (len >= sizeof(dir)) return false;
    memcpy(dir, safe_path, len + 1);

    char *last_slash = strrchr(dir, '/');
    if (!last_slash) return true;

    *last_slash = '\0';
    if (strcmp(dir, s_last_ensured_dir) == 0) return true;

    if (!ensure_dirs(dir)) return false;
    snprintf(s_last_ensured_dir, sizeof(s_last_ensured_dir), "%s", dir);
    return true;
}

bool sd_io_read(const char *path, uint8_t **out_data, size_t *out_size, size_t max_size) {
    if (!path || !out_data || !out_size) return false;

    char safe[512];
    if (!safe_path_check(path, safe, sizeof(safe))) return false;

    FILE *f = fopen(safe, "rb");
    if (!f) return false;

    if (fseek(f, 0, SEEK_END) != 0) { fclose(f); return false; }
    const long size = ftell(f);
    if (size < 0) { fclose(f); return false; }
    if (max_size > 0 && (size_t)size > max_size) { fclose(f); return false; }
    if (fseek(f, 0, SEEK_SET) != 0) { fclose(f); return false; }

    uint8_t *buf = (uint8_t *)malloc(size == 0 ? 1 : (size_t)size);
    if (!buf) { fclose(f); return false; }

    const size_t rd = fread(buf, 1, (size_t)size, f);
    fclose(f);

    if (rd != (size_t)size) {
        free(buf);
        return false;
    }

    *out_data = buf;
    *out_size = (size_t)size;
    return true;
}

void sd_io_free(uint8_t *buf) {
    free(buf);
}

SdReader *sd_io_open_read(const char *path, size_t *out_size) {
    if (!path || !out_size) return NULL;

    char safe[512];
    if (!safe_path_check(path, safe, sizeof(safe))) return NULL;

    FILE *f = fopen(safe, "rb");
    if (!f) return NULL;

    if (fseek(f, 0, SEEK_END) != 0) { fclose(f); return NULL; }
    const long size = ftell(f);
    if (size < 0) { fclose(f); return NULL; }
    if (fseek(f, 0, SEEK_SET) != 0) { fclose(f); return NULL; }

    SdReader *r = malloc(sizeof(SdReader));
    if (!r) { fclose(f); return NULL; }

    r->fp     = f;
    *out_size = (size_t)size;
    return r;
}

size_t sd_io_reader_read(SdReader *r, uint8_t *buf, size_t cap) {
    if (!r || !buf || cap == 0) return 0;
    return fread(buf, 1, cap, r->fp);
}

void sd_io_reader_close(SdReader *r) {
    if (!r) return;
    fclose(r->fp);
    free(r);
}

SdWriter *sd_io_open_write(const char *path) {
    if (!path) return NULL;

    SdWriter *w = malloc(sizeof(SdWriter));
    if (!w) return NULL;

    if (!safe_path_check(path, w->final_path, sizeof(w->final_path))) {
        free(w);
        return NULL;
    }

    if (!ensure_parent_dirs(w->final_path)) {
        free(w);
        return NULL;
    }

    if (snprintf(w->tmp_path, sizeof(w->tmp_path), "%s.tmp", w->final_path) < 0) {
        free(w);
        return NULL;
    }

    w->fp = fopen(w->tmp_path, "wb");
    if (!w->fp) {
        free(w);
        return NULL;
    }

    return w;
}

bool sd_io_writer_write(SdWriter *w, const uint8_t *data, size_t size) {
    if (!w || (!data && size > 0)) return false;
    return fwrite(data, 1, size, w->fp) == size;
}

bool sd_io_writer_close(SdWriter *w) {
    if (!w) return false;

    const bool flush_ok = (fclose(w->fp) == 0);
    w->fp = NULL;

    if (!flush_ok) {
        remove(w->tmp_path);
        free(w);
        return false;
    }

    if (rename(w->tmp_path, w->final_path) != 0) {
        remove(w->tmp_path);
        free(w);
        return false;
    }

    free(w);
    return true;
}

void sd_io_writer_abort(SdWriter *w) {
    if (!w) return;
    if (w->fp) fclose(w->fp);
    remove(w->tmp_path);
    free(w);
}

bool sd_io_log(const char *path, const char *msg) {
    if (!path || !msg) return false;

    char safe[512];
    if (!safe_path_check(path, safe, sizeof(safe))) return false;

    ensure_parent_dirs(safe);

    FILE *f = fopen(safe, "a");
    if (!f) return false;

    fputs(msg, f);
    fputc('\n', f);
    fclose(f);
    return true;
}

bool sd_io_write(const char *path, const uint8_t *data, size_t size) {
    SdWriter *w = sd_io_open_write(path);
    if (!w) return false;

    if (!sd_io_writer_write(w, data, size)) {
        sd_io_writer_abort(w);
        return false;
    }

    return sd_io_writer_close(w);
}
