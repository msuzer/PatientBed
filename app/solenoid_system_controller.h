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

enum class SolenoidPairState : uint8_t {
    OFF = 0,
    FORWARD,
    BACKWARD
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
    SolenoidSystemController();

    // Initialize PCA9685 outputs and reset controller state.
    // app_main initializes shared GPIO directions before begin(); repeating
    // GPIO setup in lower-level modules is harmless if ownership changes later.
    Result begin();

    // Configure a single pair
    // pairIndex: 0 to SOLENOID_PAIR_COUNT-1
    // config: channels and mode
    // Returns error if index out of range or channels invalid
    Result configurePair(uint8_t pairIndex, const PairConfig& config);
    Result clearPair(uint8_t pairIndex);
    Result clearAllPairs();

    // Set pair state
    // pair: 0 to SOLENOID_PAIR_COUNT-1
    // state: OFF, FORWARD, or BACKWARD
    // Returns error if pair not configured, state invalid, or I2C fails
    // Automatically updates main pump based on active pairs
    Result setPairState(uint8_t pairIndex, SolenoidPairState state);

    // Get current pair state
    SolenoidPairState getPairState(uint8_t pairIndex) const;

    // Check if any pair is active
    bool hasAnyActivePair() const;

    // Check if pair is configured and valid
    bool isPairConfigured(uint8_t pairIndex) const;

    bool isPairIdle(uint8_t pairIndex) const;

    // Manual main pump update (called automatically by setPairState)
    // Public for explicit control if needed
    Result updateMainPump();

    // Turn off configured pairs through the normal per-pair path
    Result setAllPairsOff();

    // Turn off all PCA9685 channels and main pump immediately
    Result emergencyAllOff();

    // Backward-compatible alias for emergencyAllOff.
    Result allOff();

private:
    static const uint8_t kUnassignedChannel = 0xFF;

    // Main pump GPIO control
    // Manages PIN_MAIN_PUMP_SOLENOID
    Result updateMainPumpGPIO();

    // Direct channel control helper
    Result setChannelState(uint8_t channel, bool on);

    // Validate channel index
    bool isValidChannel(uint8_t channel) const;

    // Validate pair index
    bool isValidPairIndex(uint8_t pairIndex) const;

    bool isAssignedChannel(uint8_t channel) const;
    bool isChannelUsedByOtherPair(uint8_t pairIndex, uint8_t channel) const;
    void resetPairConfig();

    Result applyComplementaryPair(const PairConfig& config, SolenoidPairState state);
    Result applyMirroredPair(const PairConfig& config, SolenoidPairState state);

    // Configuration storage
    PairConfig pairConfigs[SOLENOID_PAIR_COUNT];

    // State tracking - moved from solenoid_core
    bool sol_state[SOLENOID_CHANNEL_COUNT];  // per-channel on/off state
    SolenoidPairState pairState[SOLENOID_PAIR_COUNT];
};
