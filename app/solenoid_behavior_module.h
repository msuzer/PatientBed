#pragma once
#include <stdint.h>
#include "result.h"
#include "config.h"
#include "solenoid_behavior.h"

// ------------------------------------------------------
// Behavior module contract
// ------------------------------------------------------
struct SolenoidBehaviorOps {
    SolenoidBehaviorMode mode;
    Result (*press)(SolenoidSystemController& controller, uint8_t pairIndex, SolenoidPairState state);
    Result (*release)(SolenoidSystemController& controller, uint8_t pairIndex, SolenoidPairState state);
};

// ------------------------------------------------------
// Module factories
// ------------------------------------------------------
const SolenoidBehaviorOps *solenoid_behavior_classic_ops();
const SolenoidBehaviorOps *solenoid_behavior_shared_ops();
const SolenoidBehaviorOps *solenoid_behavior_future_ops();
