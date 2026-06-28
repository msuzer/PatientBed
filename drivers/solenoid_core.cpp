#include "solenoid.h"
#include "hal_gpio.h"
#include "pca9685.h"
#include "pins.h"

// ------------------------------------------------------
// Private state
// ------------------------------------------------------
static bool sol_state[SOLENOID_CHANNEL_COUNT] = {0};
static int8_t active_count = 0;
static bool pair_conflict_policy_enabled = true;

// ------------------------------------------------------
// Private helpers
// ------------------------------------------------------
static Result update_main_pump_solenoid() {
    Result r = hal_gpio_write(PIN_PCA_OE, active_count > 0 ? LOW : HIGH);
    if (r != RES_OK) return r;

    const bool should_on = (active_count > 0);
    r = hal_gpio_write(PIN_MAIN_PUMP_SOLENOID, should_on ? HIGH : LOW);
    if (r != RES_OK) return r;

    return RES_OK;
}

static Result solenoid_set_internal(uint8_t c, bool on) {
    const bool was_on = sol_state[c];
    if (was_on == on) return RES_OK;

    const bool is_forward = ((c % 2) == 0);
    const uint8_t other_ch = is_forward ? (c + 1) : (c - 1);

    if (pair_conflict_policy_enabled && on && sol_state[other_ch]) {
        return RES_ERR;
    }

    Result r = pca9685_setChannelState(c, on);
    if (r != RES_OK) return r;

    sol_state[c] = on;

    if (on) {
        if (active_count < SOLENOID_CHANNEL_COUNT) active_count++;
    } else {
        if (active_count > 0) active_count--;
    }

    return update_main_pump_solenoid();
}

// ------------------------------------------------------
// Public API (core)
// ------------------------------------------------------
Result solenoid_init() {
    Result r;

    r = pca9685_init();
    if (r != RES_OK) return r;

    r = hal_gpio_mode(PIN_PCA_OE, OUTPUT);
    if (r != RES_OK) return r;

    r = hal_gpio_mode(PIN_MAIN_PUMP_SOLENOID, OUTPUT);
    if (r != RES_OK) return r;

    r = hal_gpio_write(PIN_MAIN_PUMP_SOLENOID, LOW);
    if (r != RES_OK) return r;

    for (int i = 0; i < SOLENOID_CHANNEL_COUNT; i++) {
        sol_state[i] = false;
    }

    active_count = 0;
    pair_conflict_policy_enabled = true;

    return RES_OK;
}

Result solenoid_set(SolenoidChannel ch, bool on) {
    const uint8_t c = (uint8_t)ch;
    return solenoid_set_internal(c, on);
}

Result solenoid_setPairConflictPolicy(bool enabled) {
    pair_conflict_policy_enabled = enabled;
    return RES_OK;
}

Result solenoid_allOff() {
    Result r = pca9685_setAllPinsOff();
    if (r != RES_OK) return r;

    for (uint8_t c = 0; c < SOLENOID_CHANNEL_COUNT; c++) {
        sol_state[c] = false;
    }

    active_count = 0;

    r = update_main_pump_solenoid();
    if (r != RES_OK) return r;

    return RES_OK;
}
