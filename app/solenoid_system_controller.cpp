#include "solenoid_system_controller.h"
#include "logger.h"

// ======================================================
// Global singleton instance
// ======================================================
SolenoidSystemController solenoidSystem;

// ======================================================
// SolenoidSystemController implementation
// ======================================================

Result SolenoidSystemController::begin() {
    // Initialize low-level driver (PCA9685 + GPIO)
    Result r = pca9685_init();
    if (r != RES_OK) {
        log_error("PCA9685 init failed");
        return r;
    }

    r = hal_gpio_mode(PIN_MAIN_PUMP_SOLENOID, OUTPUT);
    if (r != RES_OK) return r;

    r = hal_gpio_write(PIN_MAIN_PUMP_SOLENOID, LOW);
    if (r != RES_OK) return r;

    // Initialize state tracking (moved from solenoid_core)
    for (uint8_t i = 0; i < SOLENOID_CHANNEL_COUNT; i++) {
        sol_state[i] = false;
    }
    // Initialize pair configuration and state
    for (uint8_t i = 0; i < SOLENOID_PAIR_COUNT; i++) {
        pairConfigured[i] = false;
        pairState[i] = 0;  // off
    }

    return RES_OK;
}

Result SolenoidSystemController::updateMainPumpGPIO() {
    const bool should_on = hasAnyActivePair();
    Result r = hal_gpio_write(PIN_MAIN_PUMP_SOLENOID, should_on ? HIGH : LOW);
    if (r != RES_OK) return r;

    return RES_OK;
}

Result SolenoidSystemController::setChannelState(uint8_t channel, bool on) {
    if (!isValidChannel(channel)) {
        return RES_PARAM;
    }

    if (sol_state[channel] == on) {
        return RES_OK;
    }

    Result r = pca9685_setChannelState(channel, on);
    if (r != RES_OK) return r;

    sol_state[channel] = on;
    return RES_OK;
}

// ======================================================
// Pair configuration
// ======================================================

Result SolenoidSystemController::configurePair(uint8_t pairIndex, const PairConfig& config) {
    if (!isValidPairIndex(pairIndex)) {
        log_error("Invalid pair index");
        return RES_PARAM;
    }

    if (!isValidChannel(config.forwardChannel) || !isValidChannel(config.backwardChannel)) {
        log_error("Invalid channels in config");
        return RES_PARAM;
    }

    if (config.forwardChannel == config.backwardChannel) {
        log_error("Forward and backward channels cannot be the same");
        return RES_PARAM;
    }

    pairConfigs[pairIndex] = config;
    pairConfigured[pairIndex] = true;

    log_debug("Pair configured");

    return RES_OK;
}

Result SolenoidSystemController::setPairState(uint8_t pairIndex, int8_t direction) {
    if (!isPairConfigured(pairIndex)) {
        log_error("Pair not configured");
        return RES_PARAM;
    }

    if (direction != -1 && direction != 0 && direction != 1) {
        log_error("Invalid direction");
        return RES_PARAM;
    }

    const PairConfig& config = pairConfigs[pairIndex];
    uint8_t fwd = config.forwardChannel;
    uint8_t bwd = config.backwardChannel;

    if (direction == 0) {
        // Turn off both channels
        Result r = setChannelState(fwd, false);
        if (r != RES_OK) return r;
        r = setChannelState(bwd, false);
        if (r != RES_OK) return r;
        pairState[pairIndex] = 0;

    } else if (config.mode == PairMode::COMPLEMENTARY) {
        // Opposite channels: turn one on, one off
        Result r = RES_OK;
        if (direction > 0) {
            // Forward: fwd on, bwd off
            r = setChannelState(bwd, false);
            if (r != RES_OK) return r;
            r = setChannelState(fwd, true);
            if (r != RES_OK) return r;
        } else {
            // Backward: fwd off, bwd on
            r = setChannelState(fwd, false);
            if (r != RES_OK) return r;
            r = setChannelState(bwd, true);
            if (r != RES_OK) return r;
        }

        pairState[pairIndex] = (direction > 0) ? 1 : -1;

    } else if (config.mode == PairMode::MIRRORED) {
        // Both channels together: both on or both off
        bool on = (direction != 0);
        Result r = setChannelState(fwd, on);
        if (r != RES_OK) return r;
        r = setChannelState(bwd, on);
        if (r != RES_OK) return r;
        pairState[pairIndex] = on ? (direction > 0 ? 1 : -1) : 0;
    }

    return updateMainPumpGPIO();
}

int8_t SolenoidSystemController::getPairState(uint8_t pairIndex) const {
    if (!isValidPairIndex(pairIndex)) return 0;
    return pairState[pairIndex];
}

bool SolenoidSystemController::hasAnyActivePair() const {
    for (uint8_t i = 0; i < SOLENOID_PAIR_COUNT; i++) {
        if (pairState[i] != 0) {
            return true;
        }
    }
    return false;
}

bool SolenoidSystemController::isPairConfigured(uint8_t pairIndex) const {
    if (!isValidPairIndex(pairIndex)) return false;
    return pairConfigured[pairIndex];
}

Result SolenoidSystemController::updateMainPump() {
    return updateMainPumpGPIO();
}

Result SolenoidSystemController::allOff() {
    // Bulk off via PCA9685 hardware register
    Result r = pca9685_setAllPinsOff();
    if (r != RES_OK) return r;

    // Sync software state
    for (uint8_t c = 0; c < SOLENOID_CHANNEL_COUNT; c++) {
        sol_state[c] = false;
    }

    // Reset pair state
    for (uint8_t i = 0; i < SOLENOID_PAIR_COUNT; i++) {
        pairState[i] = 0;
    }

    return updateMainPumpGPIO();
}

bool SolenoidSystemController::isValidChannel(uint8_t channel) const {
    return channel < SOLENOID_CHANNEL_COUNT;
}

bool SolenoidSystemController::isValidPairIndex(uint8_t pairIndex) const {
    return pairIndex < SOLENOID_PAIR_COUNT;
}

// ======================================================
// Legacy API wrapper - delegates to controller
// ======================================================

// For backward compatibility with app_main.cpp
Result solenoid_init() {
    return solenoidSystem.begin();
}
