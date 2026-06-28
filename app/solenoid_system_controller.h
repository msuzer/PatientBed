#pragma once
#include <stdint.h>
#include "result.h"
#include "hal_gpio.h"
#include "pca9685.h"
#include "pins.h"

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
// PairMode: How paired channels operate together
// ======================================================
enum class PairMode : uint8_t {
    COMPLEMENTARY,  // fwd and bwd channels are opposites (exclusive)
    MIRRORED        // both channels driven together (same command)
};

// ======================================================
// PairConfig: Channel mapping + mode for each pair
// ======================================================
struct PairConfig {
    uint8_t forwardChannel;   // channel index 0-15 (e.g., SOL1F)
    uint8_t backwardChannel;  // channel index 0-15 (e.g., SOL1B)
    PairMode mode;            // COMPLEMENTARY or MIRRORED
};

// ======================================================
// SolenoidSystemController: High-level pair & pump control
// ======================================================
class SolenoidSystemController {
public:
    // Initialize the system and configure all pairs
    // Must be called once at startup
    Result begin();

    // Configure a single pair
    // pairIndex: 0 to SOLENOID_PAIR_COUNT-1
    // config: channels and mode
    // Returns error if index out of range or channels invalid
    Result configurePair(uint8_t pairIndex, const PairConfig& config);

    // Set pair state based on direction
    // pair: 0 to SOLENOID_PAIR_COUNT-1
    // direction: +1 forward, -1 backward, 0 stop
    // Returns error if pair not configured, direction invalid, or I2C fails
    // Automatically updates main pump based on active pairs
    Result setPairState(uint8_t pairIndex, int8_t direction);

    // Get current pair state
    int8_t getPairState(uint8_t pairIndex) const;

    // Check if any pair is active
    bool hasAnyActivePair() const;

    // Check if pair is configured and valid
    bool isPairConfigured(uint8_t pairIndex) const;

    // Manual main pump update (called automatically by setPairState)
    // Public for explicit control if needed
    Result updateMainPump();

    // Turn off all pairs and main pump
    Result allOff();

private:
    // Main pump GPIO control
    // Manages PIN_MAIN_PUMP_SOLENOID
    Result updateMainPumpGPIO();

    // Direct channel control helper
    Result setChannelState(uint8_t channel, bool on);

    // Validate channel index
    bool isValidChannel(uint8_t channel) const;

    // Validate pair index
    bool isValidPairIndex(uint8_t pairIndex) const;

    // Configuration storage
    PairConfig pairConfigs[SOLENOID_PAIR_COUNT];
    bool pairConfigured[SOLENOID_PAIR_COUNT];

    // State tracking - moved from solenoid_core
    bool sol_state[SOLENOID_CHANNEL_COUNT];  // per-channel on/off state
    int8_t pairState[SOLENOID_PAIR_COUNT];   // 0=off, 1=fwd, -1=bwd
};

// Global singleton instance
extern SolenoidSystemController solenoidSystem;

// Backward-compatible init wrapper used by app_main.
Result solenoid_init();
