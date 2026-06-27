#pragma once
#include <stdint.h>

enum SolenoidBehaviorMode : uint8_t {
	SOL_BEHAVIOR_CLASSIC_PAIRS = 0,
	SOL_BEHAVIOR_SHARED_DIRECTION_PAIR8 = 1,
};

// Central compile-time switch for solenoid behavior.
// Set to SOL_BEHAVIOR_CLASSIC_PAIRS for original hardware.
// Set to SOL_BEHAVIOR_SHARED_DIRECTION_PAIR8 for new hardware where pair 8
// acts as global direction selector.
#ifndef SOLENOID_BEHAVIOR_MODE
#define SOLENOID_BEHAVIOR_MODE SOL_BEHAVIOR_SHARED_DIRECTION_PAIR8
#endif

