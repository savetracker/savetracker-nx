#define _POSIX_C_SOURCE 200809L // gmtime_r under -std=c11

#include "timestamp.h"

#include <stdio.h>
#include <time.h>

size_t timestamp_format(char *buf,
                        size_t cap,
                        uint64_t secs_since_epoch,
                        uint16_t millis) {
    if (!buf || cap < TIMESTAMP_BUF_SIZE) return 0;
    if (millis > 999) millis = 999;

    const time_t t = (time_t)secs_since_epoch;
    struct tm    tm;
    if (!gmtime_r(&t, &tm)) return 0;

    const int n = snprintf(buf,
                           cap,
                           "%04d%02d%02d_%02d%02d%02d_%03u",
                           tm.tm_year + 1900,
                           tm.tm_mon + 1,
                           tm.tm_mday,
                           tm.tm_hour,
                           tm.tm_min,
                           tm.tm_sec,
                           (unsigned)millis);
    if (n < 0 || (size_t)n >= cap) return 0;

    return (size_t)n;
}
