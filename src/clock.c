#include "clock.h"

#include <switch.h>

bool clock_init(void) {
    return R_SUCCEEDED(timeInitialize());
}

void clock_exit(void) {
    timeExit();
}

uint64_t clock_now_secs(void) {
    u64 secs = 0;
    if (R_FAILED(timeGetCurrentTime(TimeType_UserSystemClock, &secs))) return 0;
    return (uint64_t)secs;
}

uint16_t clock_now_millis(void) {
    const uint64_t tick = armGetSystemTick();
    const uint64_t ns   = armTicksToNs(tick);
    return (uint16_t)((ns / 1000000ULL) % 1000ULL);
}

void clock_sleep_secs(uint32_t secs) {
    svcSleepThread((uint64_t)secs * 1000000000ULL);
}
