#ifndef SAVETRACKER_TIMESTAMP_H
#define SAVETRACKER_TIMESTAMP_H

#include <stddef.h>
#include <stdint.h>

// YYYYMMDD_HHMMSS_mmm + NUL
#define TIMESTAMP_BUF_SIZE 20

// Formats UTC time as "YYYYMMDD_HHMMSS_mmm" (CopyStore-compatible).
// millis clamped to [0, 999]. Returns 19 on success, 0 on failure.
size_t timestamp_format(char *buf,
                        size_t cap,
                        uint64_t secs_since_epoch,
                        uint16_t millis);

#endif
