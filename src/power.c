#include "power.h"

#include <switch.h>

bool power_init(void) {
    return R_SUCCEEDED(psmInitialize());
}

void power_exit(void) {
    psmExit();
}

bool power_is_charging(bool *out) {
    if (!out) return false;

    PsmChargerType charger = PsmChargerType_Unconnected;
    if (R_FAILED(psmGetChargerType(&charger))) return false;

    *out = (charger != PsmChargerType_Unconnected);
    return true;
}
