#ifndef SAVETRACKER_CONFIG_H
#define SAVETRACKER_CONFIG_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef enum {
    LED_COLOR_OFF     = 0,
    LED_COLOR_WHITE   = 1,
    LED_COLOR_RED     = 2,
    LED_COLOR_YELLOW  = 3,
    LED_COLOR_GREEN   = 4,
    LED_COLOR_CYAN    = 5,
    LED_COLOR_BLUE    = 6,
    LED_COLOR_MAGENTA = 7,
} LedColor;

typedef struct {
    LedColor color;
    uint32_t duration_secs; // 0 disables this signal
} LedSignal;

typedef struct {
    uint32_t title_check_secs;
    uint32_t save_check_secs;
    uint32_t idle_secs;
    uint32_t full_snapshot_secs;
    uint32_t write_delay_secs; // wait after mtime change before reading

    LedSignal led_new_title;
    LedSignal led_error;

    bool pause_on_battery;
} Config;

void config_init_defaults(Config *cfg);

// Strict about syntax (blank, comment, or key=value only), permissive about
// unknown keys (silently ignored for forward compat). cfg should be populated
// with defaults before calling; only specified keys are overwritten.
bool config_parse(Config *cfg, const char *text, size_t len);

#endif
