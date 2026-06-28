#include "solenoid_behavior_module.h"
#include "solenoid_system_controller.h"
#include "logger.h"

static const uint8_t DIR_PAIR_IDX = SOL_DIRECTION_PAIR_INDEX;

// ------------------------------------------------------
// Private helpers
// ------------------------------------------------------
static bool any_work_pair_active(const SolenoidSystemController& controller) {
    for (uint8_t pairIndex = 0; pairIndex < SOLENOID_PAIR_COUNT; ++pairIndex) {
        if (pairIndex == DIR_PAIR_IDX) {
            continue;
        }

        if (!controller.isPairIdle(pairIndex)) {
            return true;
        }
    }
    return false;
}

static bool direction_allows_state(SolenoidPairState directionState, SolenoidPairState requestedState) {
    return directionState == SolenoidPairState::OFF ||
           directionState == requestedState;
}

static Result press(SolenoidSystemController& controller, uint8_t pairIndex, SolenoidPairState state) {
    if (pairIndex == DIR_PAIR_IDX) {
        log_info_kv("Reserved direction pair", "pair", DIR_PAIR_IDX + 1);
        return RES_NOOP;
    }

    if (!controller.isPairIdle(pairIndex)) {
        log_debug("Pair busy");
        return RES_NOOP;
    }

    const SolenoidPairState directionState = controller.getPairState(DIR_PAIR_IDX);
    if (!direction_allows_state(directionState, state)) {
        log_debug("Direction pair busy");
        return RES_NOOP;
    }

    // Activate work pair (uses MIRRORED mode, so both channels on)
    Result r = controller.setPairState(pairIndex, state);
    if (r != RES_OK) return r;

    // If direction pair is idle, activate it with same direction
    if (directionState == SolenoidPairState::OFF) {
        r = controller.setPairState(DIR_PAIR_IDX, state);
        if (r != RES_OK) {
            // Rollback work pair activation if direction pair fails
            Result rollback = controller.setPairState(pairIndex, SolenoidPairState::OFF);
            if (rollback != RES_OK) {
                log_error("Failed to rollback shared work pair");
                controller.emergencyAllOff();
            }
            return r;
        }
    }

    return RES_OK;
}

static Result release(SolenoidSystemController& controller, uint8_t pairIndex, SolenoidPairState state) {
    if (pairIndex == DIR_PAIR_IDX) {
        return RES_NOOP;
    }

    if (controller.getPairState(pairIndex) != state) {
        return RES_NOOP;
    }

    // Deactivate work pair (both channels off)
    Result r = controller.setPairState(pairIndex, SolenoidPairState::OFF);
    if (r != RES_OK) return r;

    if (!any_work_pair_active(controller)) {
        r = controller.setPairState(DIR_PAIR_IDX, SolenoidPairState::OFF);
        if (r != RES_OK) return r;
    }

    return RES_OK;
}

// ------------------------------------------------------
// Public module factory
// ------------------------------------------------------
const SolenoidBehaviorOps *solenoid_behavior_shared_ops() {
    static const SolenoidBehaviorOps ops = {
        SOL_BEHAVIOR_SHARED_DIRECTION,
        press,
        release,
    };
    return &ops;
}
