#include <Arduino.h>
#include "solenoid_behavior.h"
#include "solenoid_behavior_module.h"
#include "solenoid_system_controller.h"
#include "solenoid_system_config.h"
#include "hal_gpio.h"
#include "logger.h"

// ------------------------------------------------------
// Private state
// ------------------------------------------------------
static const SolenoidBehaviorOps *activeBehavior = nullptr;

// ------------------------------------------------------
// Private helpers
// ------------------------------------------------------
static bool is_valid_mode(SolenoidBehaviorMode mode) {
    return mode == SOL_BEHAVIOR_CLASSIC_PAIRS ||
           mode == SOL_BEHAVIOR_SHARED_DIRECTION ||
           mode == SOL_BEHAVIOR_SCENARIO_FUTURE;
}

static const SolenoidBehaviorOps *behavior_for_mode(SolenoidBehaviorMode mode) {
    if (mode == SOL_BEHAVIOR_SHARED_DIRECTION) {
        return solenoid_behavior_shared_ops();
    }
    if (mode == SOL_BEHAVIOR_SCENARIO_FUTURE) {
        return solenoid_behavior_future_ops();
    }
    return solenoid_behavior_classic_ops();
}

static const char *mode_name(SolenoidBehaviorMode mode) {
    if (mode == SOL_BEHAVIOR_SHARED_DIRECTION) {
        return "SHARED_DIRECTION";
    }
    if (mode == SOL_BEHAVIOR_SCENARIO_FUTURE) {
        return "SCENARIO_FUTURE";
    }
    return "CLASSIC_PAIRS";
}

static Result configure_pairs_for_mode(SolenoidBehaviorMode mode) {
    const PairConfig *configs = nullptr;

    if (mode == SOL_BEHAVIOR_SHARED_DIRECTION) {
        configs = SHARED_PAIR_CONFIG;
    } else if (mode == SOL_BEHAVIOR_SCENARIO_FUTURE) {
        configs = FUTURE_PAIR_CONFIG;
    } else {
        configs = CLASSIC_PAIR_CONFIG;
    }

    for (uint8_t i = 0; i < SOLENOID_PAIR_COUNT; i++) {
        Result r = solenoidSystem.configurePair(i, configs[i]);
        if (r != RES_OK) {
            log_error("Failed to configure pair");
            return r;
        }
    }

    return RES_OK;
}

static void reset_pair_state(PairState pairState[]) {
    for (uint8_t p = 1; p <= SOLENOID_PAIR_COUNT; ++p) {
        pairState[p] = PAIR_IDLE;
    }
}

static Result select_behavior_from_config() {
    SolenoidBehaviorMode activeMode = (SolenoidBehaviorMode)SOLENOID_BEHAVIOR_SELECTOR_ACTIVE_MODE;
    SolenoidBehaviorMode inactiveMode = (SolenoidBehaviorMode)SOLENOID_BEHAVIOR_SELECTOR_INACTIVE_MODE;
    if (!is_valid_mode(activeMode) || !is_valid_mode(inactiveMode)) {
        return RES_PARAM;
    }

    SolenoidBehaviorMode selectedMode = activeMode;

    if (SOLENOID_BEHAVIOR_SELECTOR_PIN != 0xFF) {
        Result r = hal_gpio_mode(
            SOLENOID_BEHAVIOR_SELECTOR_PIN,
            SOLENOID_BEHAVIOR_SELECTOR_USE_PULLUP ? INPUT_PULLUP : INPUT
        );
        if (r != RES_OK) return r;

        bool pinLevel = false;
        r = hal_gpio_read(SOLENOID_BEHAVIOR_SELECTOR_PIN, &pinLevel);
        if (r != RES_OK) return r;

        bool isActiveLevel = (pinLevel == (SOLENOID_BEHAVIOR_SELECTOR_ACTIVE_LEVEL != 0));
        selectedMode = isActiveLevel ? activeMode : inactiveMode;
    }

    activeBehavior = behavior_for_mode(selectedMode);
    return RES_OK;
}

// ------------------------------------------------------
// Public API
// ------------------------------------------------------
Result solenoid_behavior_init(PairState pairState[]) {
    if (!pairState) return RES_PARAM;

    reset_pair_state(pairState);

    Result r = select_behavior_from_config();
    if (r != RES_OK) return r;

    // Initialize the solenoid system controller
    r = solenoidSystem.begin();
    if (r != RES_OK) {
        log_error("Failed to initialize solenoid system");
        return r;
    }

    // Configure pairs based on selected scenario
    r = configure_pairs_for_mode(activeBehavior->mode);
    if (r != RES_OK) {
        log_error("Failed to configure pairs for mode");
        return r;
    }

    // Set pair conflict policy based on scenario
    log_info(mode_name(activeBehavior->mode));
    log_info_kv("Solenoid behavior mode", "mode", (int)activeBehavior->mode);
    return RES_OK;
}

SolenoidBehaviorMode solenoid_behavior_current_mode() {
    if (!activeBehavior) {
        return SOL_BEHAVIOR_CLASSIC_PAIRS;
    }
    return activeBehavior->mode;
}

Result solenoid_behavior_press(uint8_t pair, int8_t dir, PairState pairState[], bool *activated) {
    if (!activeBehavior) return RES_ERR;
    return activeBehavior->press(pair, dir, pairState, activated);
}

Result solenoid_behavior_release(uint8_t pair, int8_t dir, PairState pairState[], bool *released) {
    if (!activeBehavior) return RES_ERR;
    return activeBehavior->release(pair, dir, pairState, released);
}
