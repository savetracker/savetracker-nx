#ifndef SAVETRACKER_LED_H
#define SAVETRACKER_LED_H

#include <stdbool.h>
#include <stdint.h>

#include "../lib/config.h"

bool led_init(void);
void led_exit(void);

// No-op if color == LED_COLOR_OFF or duration_secs == 0.
void led_signal(LedColor color, uint32_t duration_secs);

#endif
