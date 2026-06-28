#pragma once
#include "solenoid_system_controller.h"

// ======================================================
// Pair configurations for each scenario mode
// ======================================================

// Classic scenario: pairs 1-7 are work pairs (complementary)
// All pairs can operate independently
static const PairConfig CLASSIC_PAIR_CONFIG[SOLENOID_PAIR_COUNT] = {
    // Pair 1
    {SOL1F, SOL1B, PairMode::COMPLEMENTARY},
    // Pair 2
    {SOL2F, SOL2B, PairMode::COMPLEMENTARY},
    // Pair 3
    {SOL3F, SOL3B, PairMode::COMPLEMENTARY},
    // Pair 4
    {SOL4F, SOL4B, PairMode::COMPLEMENTARY},
    // Pair 5
    {SOL5F, SOL5B, PairMode::COMPLEMENTARY},
    // Pair 6
    {SOL6F, SOL6B, PairMode::COMPLEMENTARY},
    // Pair 7
    {SOL7F, SOL7B, PairMode::COMPLEMENTARY},
    // Pair 8 (direction pair, still complementary for classic)
    {SOL8F, SOL8B, PairMode::COMPLEMENTARY},
};

// Shared direction scenario: pairs 1-7 are work pairs (mirrored)
// Pair 8 controls direction for all work pairs
static const PairConfig SHARED_PAIR_CONFIG[SOLENOID_PAIR_COUNT] = {
    // Pair 1 (work pair - mirrored)
    {SOL1F, SOL1B, PairMode::MIRRORED},
    // Pair 2 (work pair - mirrored)
    {SOL2F, SOL2B, PairMode::MIRRORED},
    // Pair 3 (work pair - mirrored)
    {SOL3F, SOL3B, PairMode::MIRRORED},
    // Pair 4 (work pair - mirrored)
    {SOL4F, SOL4B, PairMode::MIRRORED},
    // Pair 5 (work pair - mirrored)
    {SOL5F, SOL5B, PairMode::MIRRORED},
    // Pair 6 (work pair - mirrored)
    {SOL6F, SOL6B, PairMode::MIRRORED},
    // Pair 7 (work pair - mirrored)
    {SOL7F, SOL7B, PairMode::MIRRORED},
    // Pair 8 (direction pair - complementary, drives direction for all work)
    {SOL8F, SOL8B, PairMode::COMPLEMENTARY},
};

// Future scenario: pairs configured same as classic for now
static const PairConfig FUTURE_PAIR_CONFIG[SOLENOID_PAIR_COUNT] = {
    {SOL1F, SOL1B, PairMode::COMPLEMENTARY},
    {SOL2F, SOL2B, PairMode::COMPLEMENTARY},
    {SOL3F, SOL3B, PairMode::COMPLEMENTARY},
    {SOL4F, SOL4B, PairMode::COMPLEMENTARY},
    {SOL5F, SOL5B, PairMode::COMPLEMENTARY},
    {SOL6F, SOL6B, PairMode::COMPLEMENTARY},
    {SOL7F, SOL7B, PairMode::COMPLEMENTARY},
    {SOL8F, SOL8B, PairMode::COMPLEMENTARY},
};
