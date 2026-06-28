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

    r = hal_gpio_mode(PIN_PCA_OE, OUTPUT);
    if (r != RES_OK) return r;

    r = hal_gpio_mode(PIN_MAIN_PUMP_SOLENOID, OUTPUT);
    if (r != RES_OK) return r;

    r = hal_gpio_write(PIN_MAIN_PUMP_SOLENOID, LOW);
    if (r != RES_OK) return r;

    // Initialize state tracking (moved from solenoid_core)
    for (uint8_t i = 0; i < SOLENOID_CHANNEL_COUNT; i++) {
        sol_state[i] = false;
    }
    active_count = 0;
    pair_conflict_policy_enabled = true;

    // Initialize pair configuration and state
    for (uint8_t i = 0; i < SOLENOID_PAIR_COUNT; i++) {
        pairConfigured[i] = false;
        pairState[i] = 0;  // off
    }
    activePairCount = 0;

    return RES_OK;
}

// ======================================================
// Hardware control helpers (moved from solenoid_core)
// ======================================================

Result SolenoidSystemController::updateMainPumpGPIO() {
    Result r = hal_gpio_write(PIN_PCA_OE, active_count > 0 ? LOW : HIGH);
    if (r != RES_OK) return r;

    const bool should_on = (active_count > 0);
    r = hal_gpio_write(PIN_MAIN_PUMP_SOLENOID, should_on ? HIGH : LOW);
    if (r != RES_OK) return r;

    return RES_OK;
}

Result SolenoidSystemController::solenoid_set_internal(uint8_t c, bool on) {
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

    return updateMainPumpGPIO();
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

    Result r = RES_OK;

    if (direction == 0) {
        // Turn off both channels
        r = solenoid_set_internal(fwd, false);
        if (r != RES_OK) return r;
        r = solenoid_set_internal(bwd, false);
        if (r != RES_OK) return r;

        if (pairState[pairIndex] != 0) {
            activePairCount--;
        }
        pairState[pairIndex] = 0;

    } else if (config.mode == PairMode::COMPLEMENTARY) {
        // Opposite channels: turn one on, one off
        if (direction > 0) {
            // Forward: fwd on, bwd off
            r = solenoid_set_internal(bwd, false);
            if (r != RES_OK) return r;
            r = solenoid_set_internal(fwd, true);
            if (r != RES_OK) return r;
        } else {
            // Backward: fwd off, bwd on
            r = solenoid_set_internal(fwd, false);
            if (r != RES_OK) return r;
            r = solenoid_set_internal(bwd, true);
            if (r != RES_OK) return r;
        }

        if (pairState[pairIndex] == 0) {
            activePairCount++;
        }
        pairState[pairIndex] = (direction > 0) ? 1 : -1;

    } else if (config.mode == PairMode::MIRRORED) {
        // Both channels together: both on or both off
        bool on = (direction != 0);
        r = solenoid_set_internal(fwd, on);
        if (r != RES_OK) return r;
        r = solenoid_set_internal(bwd, on);
        if (r != RES_OK) return r;

        if (on && pairState[pairIndex] == 0) {
            activePairCount++;
        } else if (!on && pairState[pairIndex] != 0) {
            activePairCount--;
        }
        pairState[pairIndex] = on ? (direction > 0 ? 1 : -1) : 0;
    }

    // Auto-update main pump
    return updateMainPumpInternal();
}

int8_t SolenoidSystemController::getPairState(uint8_t pairIndex) const {
    if (!isValidPairIndex(pairIndex)) return 0;
    return pairState[pairIndex];
}

bool SolenoidSystemController::hasAnyActivePair() const {
    return activePairCount > 0;
}

bool SolenoidSystemController::isPairConfigured(uint8_t pairIndex) const {
    if (!isValidPairIndex(pairIndex)) return false;
    return pairConfigured[pairIndex];
}

Result SolenoidSystemController::updateMainPump() {
    return updateMainPumpInternal();
}

Result SolenoidSystemController::allOff() {
    // Bulk off via PCA9685 hardware register
    Result r = pca9685_setAllPinsOff();
    if (r != RES_OK) return r;

    // Sync software state
    for (uint8_t c = 0; c < SOLENOID_CHANNEL_COUNT; c++) {
        sol_state[c] = false;
    }
    active_count = 0;

    // Reset pair state
    for (uint8_t i = 0; i < SOLENOID_PAIR_COUNT; i++) {
        pairState[i] = 0;
    }
    activePairCount = 0;

    return updateMainPumpGPIO();
}

Result SolenoidSystemController::updateMainPumpInternal() {
    // Main pump is controlled by solenoid_set() via update_main_pump_solenoid()
    // in solenoid_core.cpp, so no additional action needed here.
    // This method is kept for future explicit pump control if needed.
    return RES_OK;
}

Result SolenoidSystemController::getChannelsForPair(uint8_t pairIndex, uint8_t& fwd, uint8_t& bwd) const {
    if (!isPairConfigured(pairIndex)) {
        return RES_PARAM;
    }
    fwd = pairConfigs[pairIndex].forwardChannel;
    bwd = pairConfigs[pairIndex].backwardChannel;
    return RES_OK;
}

bool SolenoidSystemController::isValidChannel(uint8_t channel) const {
    return channel < SOLENOID_CHANNEL_COUNT;
}

bool SolenoidSystemController::isValidPairIndex(uint8_t pairIndex) const {
    return pairIndex < SOLENOID_PAIR_COUNT;
}

Result SolenoidSystemController::setPairConflictPolicy(bool enabled) {
    pair_conflict_policy_enabled = enabled;
    return RES_OK;
}

// ======================================================
// Legacy API wrapper - delegates to controller
// ======================================================

// For backward compatibility with app_main.cpp
Result solenoid_init() {
    return solenoidSystem.begin();
}
