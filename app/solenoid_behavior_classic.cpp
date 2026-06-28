#include "solenoid_behavior_module.h"
#include "solenoid_system_controller.h"

static Result press(SolenoidSystemController& controller, uint8_t pairIndex, SolenoidPairState state) {
    if (!controller.isPairIdle(pairIndex)) {
        return RES_NOOP;
    }

    return controller.setPairState(pairIndex, state);
}

static Result release(SolenoidSystemController& controller, uint8_t pairIndex, SolenoidPairState state) {
    if (controller.getPairState(pairIndex) != state) {
        return RES_NOOP;
    }

    return controller.setPairState(pairIndex, SolenoidPairState::OFF);
}

// ------------------------------------------------------
// Public module factory
// ------------------------------------------------------
const SolenoidBehaviorOps *solenoid_behavior_classic_ops() {
    static const SolenoidBehaviorOps ops = {
        SOL_BEHAVIOR_CLASSIC_PAIRS,
        press,
        release,
    };
    return &ops;
}
