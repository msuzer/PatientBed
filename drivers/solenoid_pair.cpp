#include "solenoid.h"

// ------------------------------------------------------
// Private helpers
// ------------------------------------------------------
static bool map_pair_channels(uint8_t pair, SolenoidChannel *fwd, SolenoidChannel *bwd) {
    if (pair < 1 || pair > SOLENOID_PAIR_COUNT) return false;

    const uint8_t base = (pair - 1) * 2;
    *fwd = (SolenoidChannel)(base + 0);
    *bwd = (SolenoidChannel)(base + 1);
    return true;
}

// ------------------------------------------------------
// Public API (pair adapter)
// ------------------------------------------------------
Result solenoid_pairForward(uint8_t pair) {
    SolenoidChannel f, b;
    if (!map_pair_channels(pair, &f, &b)) return RES_PARAM;

    Result r = solenoid_set(b, false);
    if (r != RES_OK) return r;

    return solenoid_set(f, true);
}

Result solenoid_pairBackward(uint8_t pair) {
    SolenoidChannel f, b;
    if (!map_pair_channels(pair, &f, &b)) return RES_PARAM;

    Result r = solenoid_set(f, false);
    if (r != RES_OK) return r;

    return solenoid_set(b, true);
}

Result solenoid_pairStop(uint8_t pair) {
    SolenoidChannel f, b;
    if (!map_pair_channels(pair, &f, &b)) return RES_PARAM;

    Result r = solenoid_set(f, false);
    if (r != RES_OK) return r;

    return solenoid_set(b, false);
}

Result solenoid_pairSetMirrored(uint8_t pair, bool on) {
    SolenoidChannel f, b;
    if (!map_pair_channels(pair, &f, &b)) return RES_PARAM;

    Result r = solenoid_set(f, on);
    if (r != RES_OK) return r;

    r = solenoid_set(b, on);
    if (r != RES_OK) {
        solenoid_set(f, !on);
        return r;
    }

    return RES_OK;
}

Result solenoid_pairDrive(uint8_t pair, int direction) {
    if (direction > 0) return solenoid_pairForward(pair);
    if (direction < 0) return solenoid_pairBackward(pair);
    return solenoid_pairStop(pair);
}
