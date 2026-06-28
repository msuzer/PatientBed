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
    bool enforcePairConflict;
    Result (*press)(uint8_t pair, int8_t dir, PairState pairState[], bool *activated);
    Result (*release)(uint8_t pair, int8_t dir, PairState pairState[], bool *released);
};

// ------------------------------------------------------
// Module factories
// ------------------------------------------------------
const SolenoidBehaviorOps *solenoid_behavior_classic_ops();
const SolenoidBehaviorOps *solenoid_behavior_shared_ops();
const SolenoidBehaviorOps *solenoid_behavior_future_ops();
