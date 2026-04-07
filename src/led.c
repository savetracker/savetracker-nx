#include "led.h"

#include <string.h>

#include <switch.h>

bool led_init(void) {
    return R_SUCCEEDED(hidsysInitialize());
}

void led_exit(void) {
    hidsysExit();
}

static void build_pattern(HidsysNotificationLedPattern *pattern,
                          LedColor                       color,
                          uint32_t                       duration_secs) {
    memset(pattern, 0, sizeof(*pattern));

    if (color == LED_COLOR_OFF || duration_secs == 0) return;

    pattern->baseMiniCycleDuration                = 0x8;
    pattern->totalMiniCycles                      = 0x1;
    pattern->totalFullCycles                      = 0x0;
    pattern->startIntensity                       = 0xF;
    pattern->miniCycles[0].ledIntensity           = 0xF;
    pattern->miniCycles[0].transitionSteps        = 0xF;
    pattern->miniCycles[0].finalStepDuration      = (u8)duration_secs;
}

void led_signal(LedColor color, uint32_t duration_secs) {
    if (color == LED_COLOR_OFF || duration_secs == 0) return;

    HidsysNotificationLedPattern pattern;
    build_pattern(&pattern, color, duration_secs);

    s32                total = 0;
    HidsysUniquePadId  pad_ids[10];
    const s32          max_pads = (s32)(sizeof(pad_ids) / sizeof(pad_ids[0]));

    if (R_FAILED(hidsysGetUniquePadsFromNpad(HidNpadIdType_Handheld,
                                             pad_ids,
                                             max_pads,
                                             &total))) {
        return;
    }

    for (s32 i = 0; i < total && i < max_pads; i++) {
        hidsysSetNotificationLedPattern(&pattern, pad_ids[i]);
    }
}
