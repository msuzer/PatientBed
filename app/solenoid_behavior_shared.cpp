#include "solenoid_behavior_module.h"
#include "logger.h"
#include "solenoid.h"

// ------------------------------------------------------
// Private helpers
// ------------------------------------------------------
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

    Result r = solenoid_pairSetMirrored(pair, true);
    if (r != RES_OK) return r;

    if (pairState[SOL_DIRECTION_PAIR_INDEX] == PAIR_IDLE) {
        r = solenoid_pairDrive(SOL_DIRECTION_PAIR_INDEX, dir);
        if (r != RES_OK) {
            solenoid_pairSetMirrored(pair, false);
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

    Result r = solenoid_pairSetMirrored(pair, false);
    if (r != RES_OK) return r;

    pairState[pair] = PAIR_IDLE;
    *released = true;

    if (!any_work_pair_active(pairState) && pairState[SOL_DIRECTION_PAIR_INDEX] != PAIR_IDLE) {
        r = solenoid_pairStop(SOL_DIRECTION_PAIR_INDEX);
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
        false,
        press,
        release,
    };
    return &ops;
}
