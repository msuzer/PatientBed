#include "solenoid_behavior_module.h"
#include "solenoid_system_controller.h"
#include "logger.h"

// Direction pair index (0-based for controller)
static const uint8_t DIR_PAIR_IDX = SOL_DIRECTION_PAIR_INDEX - 1;

// Work pair count (direction pair is the last one)
static const uint8_t WORK_PAIR_COUNT = SOLENOID_PAIR_COUNT - 1;

// ------------------------------------------------------
// Private helpers
// ------------------------------------------------------
static bool is_valid_pair(uint8_t pair) {
    return pair >= 1 && pair <= SOLENOID_PAIR_COUNT;
}

// Convert from 1-based pair index to 0-based controller index
static uint8_t pair_to_index(uint8_t pair) {
    return pair - 1;
}

static bool any_work_pair_active(const PairState pairState[]) {
    for (uint8_t p = 1; p < SOL_DIRECTION_PAIR_INDEX; ++p) {
        if (pairState[p] != PAIR_IDLE) {
            return true;
        }
    }
    return false;
}

static Result press(uint8_t pair, int8_t dir, PairState pairState[], bool *activated) {
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

    // Activate work pair (uses MIRRORED mode, so both channels on)
    Result r = solenoidSystem.setPairState(pair_to_index(pair), dir);
    if (r != RES_OK) return r;

    // If direction pair is idle, activate it with same direction
    if (pairState[SOL_DIRECTION_PAIR_INDEX] == PAIR_IDLE) {
        r = solenoidSystem.setPairState(DIR_PAIR_IDX, dir);
        if (r != RES_OK) {
            // Rollback work pair activation if direction pair fails
            solenoidSystem.setPairState(pair_to_index(pair), 0);
            return r;
        }
        pairState[SOL_DIRECTION_PAIR_INDEX] = (dir > 0) ? PAIR_FWD : PAIR_BWD;
    }

    pairState[pair] = (dir > 0) ? PAIR_FWD : PAIR_BWD;
    *activated = true;
    return RES_OK;
}

static Result release(uint8_t pair, int8_t dir, PairState pairState[], bool *released) {
    if (!pairState || !released || !is_valid_pair(pair)) return RES_PARAM;

    *released = false;

    if (pair == SOL_DIRECTION_PAIR_INDEX) {
        return RES_OK;
    }

    if ((pairState[pair] != PAIR_FWD || dir <= 0) &&
        (pairState[pair] != PAIR_BWD || dir >= 0)) {
        return RES_OK;
    }

    // Deactivate work pair (both channels off)
    Result r = solenoidSystem.setPairState(pair_to_index(pair), 0);
    if (r != RES_OK) return r;

    pairState[pair] = PAIR_IDLE;
    *released = true;

    // If no work pairs are active, deactivate direction pair
    if (!any_work_pair_active(pairState) && pairState[SOL_DIRECTION_PAIR_INDEX] != PAIR_IDLE) {
        r = solenoidSystem.setPairState(DIR_PAIR_IDX, 0);
        if (r != RES_OK) return r;
        pairState[SOL_DIRECTION_PAIR_INDEX] = PAIR_IDLE;
    }

    return RES_OK;
}

// ------------------------------------------------------
// Public module factory
// ------------------------------------------------------
const SolenoidBehaviorOps *solenoid_behavior_shared_ops() {
    static const SolenoidBehaviorOps ops = {
        SOL_BEHAVIOR_SHARED_DIRECTION,
        press,
        release,
    };
    return &ops;
}
