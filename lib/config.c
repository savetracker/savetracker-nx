#include "config.h"

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

void config_init_defaults(Config *cfg) {
    cfg->title_check_secs   = 90;
    cfg->save_check_secs    = 15;
    cfg->idle_secs          = 90;
    cfg->full_snapshot_secs = 1800;
    cfg->write_delay_secs   = 3;

    cfg->led_new_title.color         = LED_COLOR_YELLOW;
    cfg->led_new_title.duration_secs = 3;

    cfg->led_error.color         = LED_COLOR_RED;
    cfg->led_error.duration_secs = 2;

    cfg->pause_on_battery = false;
}

typedef struct {
    const char *name;
    LedColor    color;
} ColorEntry;

static const ColorEntry COLOR_TABLE[] = {
    {"off", LED_COLOR_OFF},         {"white", LED_COLOR_WHITE},
    {"red", LED_COLOR_RED},         {"yellow", LED_COLOR_YELLOW},
    {"green", LED_COLOR_GREEN},     {"cyan", LED_COLOR_CYAN},
    {"blue", LED_COLOR_BLUE},       {"magenta", LED_COLOR_MAGENTA},
};

static size_t color_table_len(void) {
    return sizeof(COLOR_TABLE) / sizeof(COLOR_TABLE[0]);
}

static bool ieq_n(const char *a, const char *b, size_t n) {
    for (size_t i = 0; i < n; i++) {
        const int ac = tolower((unsigned char)a[i]);
        const int bc = tolower((unsigned char)b[i]);
        if (ac != bc) return false;
    }
    return b[n] == '\0';
}

static LedColor led_color_from_name(const char *name, size_t len) {
    for (size_t i = 0; i < color_table_len(); i++) {
        if (ieq_n(name, COLOR_TABLE[i].name, len)) return COLOR_TABLE[i].color;
    }
    return LED_COLOR_OFF;
}

static bool parse_uint(const char *s, size_t len, uint32_t *out) {
    if (len == 0) return false;

    uint32_t val = 0;
    for (size_t i = 0; i < len; i++) {
        if (s[i] < '0' || s[i] > '9') return false;

        const uint32_t next = val * 10 + (uint32_t)(s[i] - '0');
        if (next < val) return false;

        val = next;
    }
    *out = val;
    return true;
}

static bool parse_bool(const char *s, size_t len, bool *out) {
    if ((len == 4 && ieq_n(s, "true", 4)) || (len == 3 && ieq_n(s, "yes", 3))
        || (len == 1 && s[0] == '1')) {
        *out = true;
        return true;
    }
    if ((len == 5 && ieq_n(s, "false", 5)) || (len == 2 && ieq_n(s, "no", 2))
        || (len == 1 && s[0] == '0')) {
        *out = false;
        return true;
    }
    return false;
}

static bool parse_color(const char *s, size_t len, LedColor *out) {
    LedColor c = led_color_from_name(s, len);
    if (c == LED_COLOR_OFF && !(len == 3 && ieq_n(s, "off", 3))) return false;
    *out = c;
    return true;
}

static void trim(const char **start, size_t *len) {
    const char *s = *start;
    size_t      n = *len;
    while (n > 0 && isspace((unsigned char)s[0])) {
        s++;
        n--;
    }
    while (n > 0 && isspace((unsigned char)s[n - 1])) n--;
    *start = s;
    *len   = n;
}

static bool slice_eq(const char *s, size_t n, const char *lit) {
    return strlen(lit) == n && memcmp(s, lit, n) == 0;
}

static bool apply_kv(Config *cfg,
                     const char *key, size_t klen,
                     const char *val, size_t vlen) {
    if (slice_eq(key, klen, "title_check_secs")) return parse_uint(val, vlen, &cfg->title_check_secs);
    if (slice_eq(key, klen, "save_check_secs")) return parse_uint(val, vlen, &cfg->save_check_secs);
    if (slice_eq(key, klen, "idle_secs")) return parse_uint(val, vlen, &cfg->idle_secs);
    if (slice_eq(key, klen, "full_snapshot_secs")) return parse_uint(val, vlen, &cfg->full_snapshot_secs);
    if (slice_eq(key, klen, "write_delay_secs")) return parse_uint(val, vlen, &cfg->write_delay_secs);
    if (slice_eq(key, klen, "led_new_title_color")) return parse_color(val, vlen, &cfg->led_new_title.color);
    if (slice_eq(key, klen, "led_new_title_duration")) return parse_uint(val, vlen, &cfg->led_new_title.duration_secs);
    if (slice_eq(key, klen, "led_error_color")) return parse_color(val, vlen, &cfg->led_error.color);
    if (slice_eq(key, klen, "led_error_duration")) return parse_uint(val, vlen, &cfg->led_error.duration_secs);
    if (slice_eq(key, klen, "pause_on_battery")) return parse_bool(val, vlen, &cfg->pause_on_battery);

    return true; // unknown key — forward compat
}

bool config_parse(Config *cfg, const char *text, size_t len) {
    size_t pos = 0;

    while (pos < len) {
        const size_t line_start = pos;
        while (pos < len && text[pos] != '\n') pos++;

        size_t      line_len = pos - line_start;
        const char *line     = text + line_start;
        if (pos < len) pos++;

        if (line_len > 0 && line[line_len - 1] == '\r') line_len--;

        trim(&line, &line_len);

        if (line_len == 0 || line[0] == '#') continue;

        const char *eq = memchr(line, '=', line_len);
        if (!eq) return false;

        const char *key  = line;
        size_t      klen = (size_t)(eq - line);
        const char *val  = eq + 1;
        size_t      vlen = line_len - klen - 1;

        trim(&key, &klen);
        trim(&val, &vlen);

        if (klen == 0) return false;
        if (!apply_kv(cfg, key, klen, val, vlen)) return false;
    }

    return true;
}
