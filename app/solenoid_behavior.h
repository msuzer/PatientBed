#pragma once
#include <stdint.h>
#include "config.h"
#include "result.h"

enum PairState : uint8_t { PAIR_IDLE = 0, PAIR_FWD, PAIR_BWD };

Result solenoid_behavior_init(PairState pairState[]);
SolenoidBehaviorMode solenoid_behavior_current_mode();
Result solenoid_behavior_press(uint8_t pair, int8_t dir, PairState pairState[], bool *activated);
Result solenoid_behavior_release(uint8_t pair, int8_t dir, PairState pairState[], bool *released);
