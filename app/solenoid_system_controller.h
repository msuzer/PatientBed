#pragma once
#include <stdint.h>
#include "result.h"
#include "solenoid.h"
#include "hal_gpio.h"
#include "pca9685.h"
#include "pins.h"

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

    // Set pair conflict policy (complementary pairs cannot both be on)
    Result setPairConflictPolicy(bool enabled);

private:
    // Main pump automation: turn on if any pair active, off if all idle
    Result updateMainPumpInternal();

    // Direct channel control with conflict checking
    // c: channel index 0-15
    // on: true to activate, false to deactivate
    Result solenoid_set_internal(uint8_t c, bool on);

    // Main pump GPIO control
    // Manages PIN_PCA_OE (output enable) and PIN_MAIN_PUMP_SOLENOID
    Result updateMainPumpGPIO();

    // Get forward and backward channels for a pair
    Result getChannelsForPair(uint8_t pairIndex, uint8_t& fwd, uint8_t& bwd) const;

    // Validate channel index
    bool isValidChannel(uint8_t channel) const;

    // Validate pair index
    bool isValidPairIndex(uint8_t pairIndex) const;

    // Configuration storage
    PairConfig pairConfigs[SOLENOID_PAIR_COUNT];
    bool pairConfigured[SOLENOID_PAIR_COUNT];

    // State tracking - moved from solenoid_core
    bool sol_state[SOLENOID_CHANNEL_COUNT];  // per-channel on/off state
    int8_t active_count;                     // count of active channels (drives main pump)
    bool pair_conflict_policy_enabled;       // whether to enforce pair conflicts
    int8_t pairState[SOLENOID_PAIR_COUNT];   // 0=off, 1=fwd, -1=bwd
    uint8_t activePairCount;                 // count of non-idle pairs
};

// Global singleton instance
extern SolenoidSystemController solenoidSystem;
