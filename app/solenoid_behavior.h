#pragma once
#include <stdint.h>
#include "config.h"
#include "result.h"
#include "solenoid_system_controller.h"

Result solenoid_behavior_init(SolenoidSystemController& controller);
SolenoidBehaviorMode solenoid_behavior_current_mode();
Result solenoid_behavior_press(uint8_t pairIndex, SolenoidPairState state);
Result solenoid_behavior_release(uint8_t pairIndex, SolenoidPairState state);
