#pragma once

#include <stdint.h>
#include "result.h"
#include "solenoid_system_controller.h"

struct InputRouterEvent {
    bool recognized;
    bool accepted;
    uint8_t pairIndex;
    SolenoidPairState state;
    bool pressed;
};

bool input_router_is_bound_key(uint8_t keyId);
Result input_router_handle_key_event(uint8_t keyId, bool pressed, InputRouterEvent *event);
