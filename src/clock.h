#ifndef SAVETRACKER_CLOCK_H
#define SAVETRACKER_CLOCK_H

#include <stdbool.h>
#include <stdint.h>

bool clock_init(void);
void clock_exit(void);

uint64_t clock_now_secs(void);

// Tiebreaker for snapshots in the same second, [0, 999].
uint16_t clock_now_millis(void);

void clock_sleep_secs(uint32_t secs);

#endif
