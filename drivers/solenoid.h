#pragma once
#include "result.h"
#include <stdint.h>

#ifndef SOLENOID_CHANNEL_COUNT
#define SOLENOID_CHANNEL_COUNT 16
#endif

#ifndef SOLENOID_PAIR_COUNT
#define SOLENOID_PAIR_COUNT (SOLENOID_CHANNEL_COUNT / 2)
#endif

// ------------------------------------------------------
// Channel definitions
// ------------------------------------------------------
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

// ======================================================
// Legacy API - for backward compatibility
// Delegates to SolenoidSystemController
// ======================================================

// Initialize solenoid system (delegates to solenoidSystem.begin())
Result solenoid_init();
