#include "solenoid_behavior_module.h"

// Scenario 3 scaffold.
// Fill in press/release behavior here when the scenario is defined.

// ------------------------------------------------------
// Private handlers (scaffold)
// ------------------------------------------------------

static Result press(uint8_t pair, int8_t dir, PairState pairState[], bool *activated) {
    // TODO: Validate pair/dir and reject unsupported combinations.
    // TODO: Apply any occupancy/conflict checks before actuating outputs.
    // TODO: Drive hardware for press and update pairState accordingly.
    (void)pair;
    (void)dir;
    (void)pairState;

    if (!activated) return RES_PARAM;
    *activated = false;

    return RES_OK;
}

static Result release(uint8_t pair, int8_t dir, PairState pairState[], bool *released) {
    // TODO: Verify that this release matches the currently active direction.
    // TODO: Stop/deactivate hardware outputs for this scenario.
    // TODO: Restore pairState and perform shared-resource cleanup if needed.
    (void)pair;
    (void)dir;
    (void)pairState;

    if (!released) return RES_PARAM;
    *released = false;

    return RES_OK;
}

// ------------------------------------------------------
// Public module factory
// ------------------------------------------------------
const SolenoidBehaviorOps *solenoid_behavior_future_ops() {
    static const SolenoidBehaviorOps ops = {
        SOL_BEHAVIOR_SCENARIO_FUTURE,
        press,
        release,
    };
    return &ops;
}
