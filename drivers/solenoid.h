#pragma once
#include "result.h"
#include <stdint.h>

// Solenoid channel mapping (PCA9685)
enum SolenoidChannel : uint8_t {
    SOL1F = 0,
    SOL1B = 1,
    SOL2F = 2,
    SOL2B = 3,
    SOL3F = 4,
    SOL3B = 5,
    SOL4F = 6,
    SOL4B = 7,
    SOL5F = 8,
    SOL5B = 9,
    SOL6F = 10,
    SOL6B = 11,
    SOL7F = 12,
    SOL7B = 13,
    SOL8F = 14,
    SOL8B = 15
};

// Public API
Result solenoid_init();

// Controls individual channel (1F/1B/etc)
Result solenoid_set(SolenoidChannel ch, bool on);

// Controls pair
Result solenoid_forward(uint8_t pair);  // 1–8
Result solenoid_backward(uint8_t pair); // 1–8
Result solenoid_stopPair(uint8_t pair); // turns both off
Result solenoid_mirrorPair(uint8_t pair, bool on); // drives both pair outputs together

// Unified API (dir = +1 → forward, -1 → backward, 0 → stop)
Result solenoid_drive(uint8_t pair, int direction);

// Global controls
Result solenoid_allOff();

// Automatic Sol0 management is internal
