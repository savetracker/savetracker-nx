#ifndef SAVETRACKER_POWER_H
#define SAVETRACKER_POWER_H

#include <stdbool.h>

bool power_init(void);
void power_exit(void);

// Writes true to *out if plugged in. Returns false on query error.
bool power_is_charging(bool *out);

#endif
