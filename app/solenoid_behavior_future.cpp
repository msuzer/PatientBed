#include "solenoid_behavior_module.h"

// Scenario 3 scaffold.
// Fill in press/release behavior here when the scenario is defined.

// ------------------------------------------------------
// Private handlers (scaffold)
// ------------------------------------------------------

static Result press(SolenoidSystemController& controller, uint8_t pair, SolenoidPairState state) {
    // TODO: Validate pair/dir and reject unsupported combinations.
    // TODO: Apply any occupancy/conflict checks before actuating outputs.
    // TODO: Drive hardware for press and update controller state accordingly.
    (void)pair;
    (void)state;
    (void)controller;

    return RES_NOOP;
}

static Result release(SolenoidSystemController& controller, uint8_t pair, SolenoidPairState state) {
    // TODO: Verify that this release matches the currently active direction.
    // TODO: Stop/deactivate hardware outputs for this scenario.
    // TODO: Restore controller state and perform shared-resource cleanup if needed.
    (void)pair;
    (void)state;
    (void)controller;

    return RES_NOOP;
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
