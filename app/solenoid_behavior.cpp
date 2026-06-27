#include <Arduino.h>
#include "solenoid_behavior.h"
#include "hal_gpio.h"
#include "logger.h"
#include "solenoid.h"

struct SolenoidBehaviorOps {
    SolenoidBehaviorMode mode;
    Result (*press)(uint8_t pair, int8_t dir, PairState pairState[], bool *activated);
    Result (*release)(uint8_t pair, int8_t dir, PairState pairState[], bool *released);
};

static Result classic_press(uint8_t pair, int8_t dir, PairState pairState[], bool *activated);
static Result classic_release(uint8_t pair, int8_t dir, PairState pairState[], bool *released);
static Result shared_press(uint8_t pair, int8_t dir, PairState pairState[], bool *activated);
static Result shared_release(uint8_t pair, int8_t dir, PairState pairState[], bool *released);

static const SolenoidBehaviorOps CLASSIC_BEHAVIOR = {
    SOL_BEHAVIOR_CLASSIC_PAIRS,
    classic_press,
    classic_release,
};

static const SolenoidBehaviorOps SHARED_DIRECTION_BEHAVIOR = {
    SOL_BEHAVIOR_SHARED_DIRECTION,
    shared_press,
    shared_release,
};

static const SolenoidBehaviorOps *activeBehavior = &SHARED_DIRECTION_BEHAVIOR;

static bool is_valid_pair(uint8_t pair) {
    return pair >= 1 && pair <= SOLENOID_PAIR_COUNT;
}

static bool any_work_pair_active(const PairState pairState[]) {
    for (uint8_t p = 1; p <= SOLENOID_PAIR_COUNT; ++p) {
        if (p == SOL_DIRECTION_PAIR_INDEX) continue;
        if (pairState[p] != PAIR_IDLE) {
            return true;
        }
    }
    return false;
}

static bool is_valid_mode(SolenoidBehaviorMode mode) {
    return mode == SOL_BEHAVIOR_CLASSIC_PAIRS ||
           mode == SOL_BEHAVIOR_SHARED_DIRECTION;
}

static SolenoidBehaviorMode other_mode(SolenoidBehaviorMode mode) {
    return (mode == SOL_BEHAVIOR_SHARED_DIRECTION)
        ? SOL_BEHAVIOR_CLASSIC_PAIRS
        : SOL_BEHAVIOR_SHARED_DIRECTION;
}

static const SolenoidBehaviorOps *behavior_for_mode(SolenoidBehaviorMode mode) {
    return (mode == SOL_BEHAVIOR_SHARED_DIRECTION)
        ? &SHARED_DIRECTION_BEHAVIOR
        : &CLASSIC_BEHAVIOR;
}

static const char *mode_name(SolenoidBehaviorMode mode) {
    if (mode == SOL_BEHAVIOR_SHARED_DIRECTION) {
        return "SHARED_DIRECTION";
    }
    return "CLASSIC_PAIRS";
}

static void reset_pair_state(PairState pairState[]) {
    for (uint8_t p = 1; p <= SOLENOID_PAIR_COUNT; ++p) {
        pairState[p] = PAIR_IDLE;
    }
}

static Result select_behavior_from_config() {
    SolenoidBehaviorMode selectedMode = (SolenoidBehaviorMode)SOLENOID_BEHAVIOR_SELECTOR_ACTIVE_MODE;
    if (!is_valid_mode(selectedMode)) {
        return RES_PARAM;
    }

    if (SOLENOID_BEHAVIOR_SELECTOR_PIN != 0xFF) {
        Result r = hal_gpio_mode(
            SOLENOID_BEHAVIOR_SELECTOR_PIN,
            SOLENOID_BEHAVIOR_SELECTOR_USE_PULLUP ? INPUT_PULLUP : INPUT
        );
        if (r != RES_OK) return r;

        bool pinLevel = false;
        r = hal_gpio_read(SOLENOID_BEHAVIOR_SELECTOR_PIN, &pinLevel);
        if (r != RES_OK) return r;

        bool isActiveLevel = (pinLevel == (SOLENOID_BEHAVIOR_SELECTOR_ACTIVE_LEVEL != 0));
        SolenoidBehaviorMode activeMode = (SolenoidBehaviorMode)SOLENOID_BEHAVIOR_SELECTOR_ACTIVE_MODE;
        if (!is_valid_mode(activeMode)) {
            return RES_PARAM;
        }

        selectedMode = isActiveLevel ? activeMode : other_mode(activeMode);
    }

    activeBehavior = behavior_for_mode(selectedMode);
    return RES_OK;
}

Result solenoid_behavior_init(PairState pairState[]) {
    if (!pairState) return RES_PARAM;

    reset_pair_state(pairState);

    Result r = select_behavior_from_config();
    if (r != RES_OK) return r;

    log_info(mode_name(activeBehavior->mode));
    log_info_kv("Solenoid behavior mode", "mode", (int)activeBehavior->mode);
    return RES_OK;
}

SolenoidBehaviorMode solenoid_behavior_current_mode() {
    return activeBehavior->mode;
}

static Result classic_press(uint8_t pair, int8_t dir, PairState pairState[], bool *activated) {
    if (!pairState || !activated || !is_valid_pair(pair)) return RES_PARAM;

    *activated = false;

    if (pairState[pair] != PAIR_IDLE) {
        log_debug("Pair busy");
        return RES_OK;
    }

    Result r = solenoid_drive(pair, dir);
    if (r != RES_OK) return r;

    pairState[pair] = (dir > 0) ? PAIR_FWD : PAIR_BWD;
    *activated = true;
    return RES_OK;
}

static Result classic_release(uint8_t pair, int8_t dir, PairState pairState[], bool *released) {
    if (!pairState || !released || !is_valid_pair(pair)) return RES_PARAM;

    *released = false;

    if ((pairState[pair] != PAIR_FWD || dir <= 0) &&
        (pairState[pair] != PAIR_BWD || dir >= 0)) {
        return RES_OK;
    }

    Result r = solenoid_stopPair(pair);
    if (r != RES_OK) return r;

    pairState[pair] = PAIR_IDLE;
    *released = true;

    return RES_OK;
}

static Result shared_press(uint8_t pair, int8_t dir, PairState pairState[], bool *activated) {
    if (!pairState || !activated || !is_valid_pair(pair)) return RES_PARAM;

    *activated = false;

    if (pair == SOL_DIRECTION_PAIR_INDEX) {
        log_info_kv("Reserved direction pair", "pair", SOL_DIRECTION_PAIR_INDEX);
        return RES_OK;
    }

    if (pairState[pair] != PAIR_IDLE) {
        log_debug("Pair busy");
        return RES_OK;
    }

    if ((pairState[SOL_DIRECTION_PAIR_INDEX] == PAIR_FWD && dir < 0) ||
        (pairState[SOL_DIRECTION_PAIR_INDEX] == PAIR_BWD && dir > 0)) {
        log_debug("Direction pair busy");
        return RES_OK;
    }

    Result r = solenoid_mirrorPair(pair, true);
    if (r != RES_OK) return r;

    if (pairState[SOL_DIRECTION_PAIR_INDEX] == PAIR_IDLE) {
        r = solenoid_drive(SOL_DIRECTION_PAIR_INDEX, dir);
        if (r != RES_OK) {
            solenoid_mirrorPair(pair, false);
            return r;
        }
        pairState[SOL_DIRECTION_PAIR_INDEX] = (dir > 0) ? PAIR_FWD : PAIR_BWD;
    }

    pairState[pair] = (dir > 0) ? PAIR_FWD : PAIR_BWD;
    *activated = true;
    return RES_OK;
}

static Result shared_release(uint8_t pair, int8_t dir, PairState pairState[], bool *released) {
    if (!pairState || !released || !is_valid_pair(pair)) return RES_PARAM;

    *released = false;

    if (pair == SOL_DIRECTION_PAIR_INDEX) {
        return RES_OK;
    }

    if ((pairState[pair] != PAIR_FWD || dir <= 0) &&
        (pairState[pair] != PAIR_BWD || dir >= 0)) {
        return RES_OK;
    }

    Result r = solenoid_mirrorPair(pair, false);
    if (r != RES_OK) return r;

    pairState[pair] = PAIR_IDLE;
    *released = true;

    if (!any_work_pair_active(pairState) && pairState[SOL_DIRECTION_PAIR_INDEX] != PAIR_IDLE) {
        r = solenoid_stopPair(SOL_DIRECTION_PAIR_INDEX);
        if (r != RES_OK) return r;
        pairState[SOL_DIRECTION_PAIR_INDEX] = PAIR_IDLE;
    }

    return RES_OK;
}

Result solenoid_behavior_press(uint8_t pair, int8_t dir, PairState pairState[], bool *activated) {
    return activeBehavior->press(pair, dir, pairState, activated);
}

Result solenoid_behavior_release(uint8_t pair, int8_t dir, PairState pairState[], bool *released) {
    return activeBehavior->release(pair, dir, pairState, released);
}
