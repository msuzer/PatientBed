#pragma once
#include <stdint.h>
#include <Arduino.h>

enum SolenoidBehaviorMode : uint8_t {
	SOL_BEHAVIOR_CLASSIC_PAIRS = 0,
	SOL_BEHAVIOR_SHARED_DIRECTION = 1,
};

#ifndef SOL_DIRECTION_PAIR_INDEX
#define SOL_DIRECTION_PAIR_INDEX 8
#endif

// PB4 (Arduino D12) is used as mode selector because current active keypad
// key IDs do not use this matrix line. The line is externally pulled up and
// a jumper to GND selects the alternate behavior mode at boot.
// Optional boot-time selector pin. Leave at 0xFF to use
// SOLENOID_BEHAVIOR_SELECTOR_ACTIVE_MODE always.
#ifndef SOLENOID_BEHAVIOR_SELECTOR_PIN
#define SOLENOID_BEHAVIOR_SELECTOR_PIN 12
#endif

// Selector pin electrical setup and mapping.
// When the sampled pin level equals SOLENOID_BEHAVIOR_SELECTOR_ACTIVE_LEVEL,
// the active mode becomes SOLENOID_BEHAVIOR_SELECTOR_ACTIVE_MODE. Otherwise,
// the other defined mode is used.
#ifndef SOLENOID_BEHAVIOR_SELECTOR_USE_PULLUP
#define SOLENOID_BEHAVIOR_SELECTOR_USE_PULLUP 0
#endif

#ifndef SOLENOID_BEHAVIOR_SELECTOR_ACTIVE_LEVEL
#define SOLENOID_BEHAVIOR_SELECTOR_ACTIVE_LEVEL 0
#endif

#ifndef SOLENOID_BEHAVIOR_SELECTOR_ACTIVE_MODE
#define SOLENOID_BEHAVIOR_SELECTOR_ACTIVE_MODE SOL_BEHAVIOR_SHARED_DIRECTION
#endif

