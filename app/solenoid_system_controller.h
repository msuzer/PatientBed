#pragma once
#include <stdint.h>
#include "result.h"
#include "solenoid.h"

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
    // Main pump automation: turn on if any pair active, off if all idle
    Result updateMainPumpInternal();

    // Get forward and backward channels for a pair
    Result getChannelsForPair(uint8_t pairIndex, uint8_t& fwd, uint8_t& bwd) const;

    // Validate channel index
    bool isValidChannel(uint8_t channel) const;

    // Validate pair index
    bool isValidPairIndex(uint8_t pairIndex) const;

    // Configuration storage
    PairConfig pairConfigs[SOLENOID_PAIR_COUNT];
    bool pairConfigured[SOLENOID_PAIR_COUNT];

    // State tracking
    int8_t pairState[SOLENOID_PAIR_COUNT];  // 0=off, 1=fwd, -1=bwd
    uint8_t activePairCount;                // count of non-idle pairs
};

// Global singleton instance
extern SolenoidSystemController solenoidSystem;
