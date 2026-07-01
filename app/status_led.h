#pragma once

#include <stdint.h>

enum class StatusLedMode : uint8_t {
    NORMAL = 0,
    CHARGING,
    ERROR
};

void status_led_setMode(StatusLedMode mode);
StatusLedMode status_led_getMode();
void status_led_tick10ms();
