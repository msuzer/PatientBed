#pragma once
#include <stdint.h>

enum SolenoidBehaviorMode : uint8_t {
	SOL_BEHAVIOR_CLASSIC_PAIRS = 0,
	SOL_BEHAVIOR_SHARED_DIRECTION_PAIR8 = 1,
};

// Central compile-time switch for solenoid behavior.
// Set to SOL_BEHAVIOR_CLASSIC_PAIRS for original hardware.
// Set to SOL_BEHAVIOR_SHARED_DIRECTION_PAIR8 for hardware where pairs 1..7
// are mirrored ON/OFF valve outputs and pair 8 acts as the global direction
// selector for them.
#ifndef SOLENOID_BEHAVIOR_MODE
#define SOLENOID_BEHAVIOR_MODE SOL_BEHAVIOR_SHARED_DIRECTION_PAIR8
#endif

#ifndef SOL_SHARED_DIRECTION_PAIR_INDEX
#define SOL_SHARED_DIRECTION_PAIR_INDEX 8
#endif

