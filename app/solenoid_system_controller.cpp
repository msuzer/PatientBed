#include "solenoid_system_controller.h"
#include "logger.h"

// ======================================================
// SolenoidSystemController implementation
// ======================================================

SolenoidSystemController::SolenoidSystemController() {
    for (uint8_t c = 0; c < SOLENOID_CHANNEL_COUNT; c++) {
        sol_state[c] = false;
    }

    resetPairConfig();
}

Result SolenoidSystemController::begin() {
    // Initialize low-level driver (PCA9685 + GPIO)
    Result r = pca9685_init();
    if (r != RES_OK) {
        log_error("PCA9685 init failed");
        return r;
    }

    r = pca9685_setAllPinsOff();
    if (r != RES_OK) return r;

    for (uint8_t i = 0; i < SOLENOID_CHANNEL_COUNT; i++) {
        sol_state[i] = false;
    }

    resetPairConfig();

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

    if (pairState[pairIndex] != SolenoidPairState::OFF) {
        log_error("Cannot configure active pair");
        return RES_BUSY;
    }

    if (isChannelUsedByOtherPair(pairIndex, config.forwardChannel) ||
        isChannelUsedByOtherPair(pairIndex, config.backwardChannel)) {
        log_error("Channel already assigned to another pair");
        return RES_PARAM;
    }

    pairConfigs[pairIndex] = config;
    pairState[pairIndex] = SolenoidPairState::OFF;

    log_debug("Pair configured");

    return RES_OK;
}

Result SolenoidSystemController::setPairState(uint8_t pairIndex, SolenoidPairState state) {
    if (!isPairConfigured(pairIndex)) {
        log_error("Pair not configured");
        return RES_PARAM;
    }

    const PairConfig& config = pairConfigs[pairIndex];
    Result r = RES_OK;

    switch (config.mode) {
        case PairMode::COMPLEMENTARY:
            r = applyComplementaryPair(config, state);
            break;

        case PairMode::MIRRORED:
            r = applyMirroredPair(config, state);
            break;

        default:
            log_error("Invalid pair mode");
            return RES_PARAM;
    }

    if (r != RES_OK) return r;

    pairState[pairIndex] = state;
    return updateMainPumpGPIO();
}

SolenoidPairState SolenoidSystemController::getPairState(uint8_t pairIndex) const {
    if (!isValidPairIndex(pairIndex)) return SolenoidPairState::OFF;
    return pairState[pairIndex];
}

bool SolenoidSystemController::hasAnyActivePair() const {
    for (uint8_t i = 0; i < SOLENOID_PAIR_COUNT; i++) {
        if (isPairConfigured(i) && pairState[i] != SolenoidPairState::OFF) {
            return true;
        }
    }
    return false;
}

bool SolenoidSystemController::isPairConfigured(uint8_t pairIndex) const {
    if (!isValidPairIndex(pairIndex)) return false;
    return isAssignedChannel(pairConfigs[pairIndex].forwardChannel) &&
           isAssignedChannel(pairConfigs[pairIndex].backwardChannel);
}

bool SolenoidSystemController::isPairIdle(uint8_t pairIndex) const {
    return isValidPairIndex(pairIndex) &&
           pairState[pairIndex] == SolenoidPairState::OFF;
}

Result SolenoidSystemController::updateMainPump() {
    return updateMainPumpGPIO();
}

Result SolenoidSystemController::allOff() {
    return emergencyAllOff();
}

Result SolenoidSystemController::setAllPairsOff() {
    for (uint8_t i = 0; i < SOLENOID_PAIR_COUNT; i++) {
        if (!isPairConfigured(i)) {
            continue;
        }

        Result r = setPairState(i, SolenoidPairState::OFF);
        if (r != RES_OK) return r;
    }

    return RES_OK;
}

Result SolenoidSystemController::emergencyAllOff() {
    // Bulk off via PCA9685 hardware register
    Result r = pca9685_setAllPinsOff();
    if (r != RES_OK) return r;

    // Sync software state
    for (uint8_t c = 0; c < SOLENOID_CHANNEL_COUNT; c++) {
        sol_state[c] = false;
    }

    // Reset pair state
    for (uint8_t i = 0; i < SOLENOID_PAIR_COUNT; i++) {
        pairState[i] = SolenoidPairState::OFF;
    }

    return updateMainPumpGPIO();
}

Result SolenoidSystemController::clearPair(uint8_t pairIndex) {
    if (!isValidPairIndex(pairIndex)) {
        return RES_PARAM;
    }

    if (pairState[pairIndex] != SolenoidPairState::OFF) {
        return RES_BUSY;
    }

    pairConfigs[pairIndex].forwardChannel = kUnassignedChannel;
    pairConfigs[pairIndex].backwardChannel = kUnassignedChannel;
    pairConfigs[pairIndex].mode = PairMode::COMPLEMENTARY;
    return RES_OK;
}

Result SolenoidSystemController::clearAllPairs() {
    if (hasAnyActivePair()) {
        return RES_BUSY;
    }

    resetPairConfig();
    return RES_OK;
}

void SolenoidSystemController::resetPairConfig() {
    for (uint8_t i = 0; i < SOLENOID_PAIR_COUNT; i++) {
        pairState[i] = SolenoidPairState::OFF;
        pairConfigs[i].forwardChannel = kUnassignedChannel;
        pairConfigs[i].backwardChannel = kUnassignedChannel;
        pairConfigs[i].mode = PairMode::COMPLEMENTARY;
    }
}

bool SolenoidSystemController::isValidChannel(uint8_t channel) const {
    return channel < SOLENOID_CHANNEL_COUNT;
}

bool SolenoidSystemController::isValidPairIndex(uint8_t pairIndex) const {
    return pairIndex < SOLENOID_PAIR_COUNT;
}

bool SolenoidSystemController::isAssignedChannel(uint8_t channel) const {
    return channel != kUnassignedChannel;
}

bool SolenoidSystemController::isChannelUsedByOtherPair(uint8_t pairIndex, uint8_t channel) const {
    for (uint8_t i = 0; i < SOLENOID_PAIR_COUNT; i++) {
        if (i == pairIndex || !isPairConfigured(i)) {
            continue;
        }

        if (pairConfigs[i].forwardChannel == channel ||
            pairConfigs[i].backwardChannel == channel) {
            return true;
        }
    }

    return false;
}

Result SolenoidSystemController::applyComplementaryPair(const PairConfig& config, SolenoidPairState state) {
    switch (state) {
        case SolenoidPairState::OFF: {
            Result r = setChannelState(config.forwardChannel, false);
            if (r != RES_OK) return r;
            return setChannelState(config.backwardChannel, false);
        }

        case SolenoidPairState::FORWARD: {
            Result r = setChannelState(config.backwardChannel, false);
            if (r != RES_OK) return r;
            return setChannelState(config.forwardChannel, true);
        }

        case SolenoidPairState::BACKWARD: {
            Result r = setChannelState(config.forwardChannel, false);
            if (r != RES_OK) return r;
            return setChannelState(config.backwardChannel, true);
        }

        default:
            return RES_PARAM;
    }
}

Result SolenoidSystemController::applyMirroredPair(const PairConfig& config, SolenoidPairState state) {
    if (state != SolenoidPairState::OFF &&
        state != SolenoidPairState::FORWARD &&
        state != SolenoidPairState::BACKWARD) {
        return RES_PARAM;
    }

    const bool on = (state != SolenoidPairState::OFF);
    Result r = setChannelState(config.forwardChannel, on);
    if (r != RES_OK) return r;

    return setChannelState(config.backwardChannel, on);
}
