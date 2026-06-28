#include "solenoid_behavior_module.h"
#include "solenoid_system_controller.h"
#include "logger.h"

// ------------------------------------------------------
// Private helpers
// ------------------------------------------------------
static bool is_valid_pair(uint8_t pair) {
    return pair >= 1 && pair <= SOLENOID_PAIR_COUNT;
}

// Convert from 1-based pair index to 0-based SolenoidSystemController index
static uint8_t pair_to_index(uint8_t pair) {
    return pair - 1;
}

static Result press(uint8_t pair, int8_t dir, PairState pairState[], bool *activated) {
    if (!pairState || !activated || !is_valid_pair(pair)) return RES_PARAM;

    *activated = false;

    if (pairState[pair] != PAIR_IDLE) {
        log_debug("Pair busy");
        return RES_OK;
    }

    Result r = solenoidSystem.setPairState(pair_to_index(pair), dir);
    if (r != RES_OK) return r;

    pairState[pair] = (dir > 0) ? PAIR_FWD : PAIR_BWD;
    *activated = true;
    return RES_OK;
}

static Result release(uint8_t pair, int8_t dir, PairState pairState[], bool *released) {
    if (!pairState || !released || !is_valid_pair(pair)) return RES_PARAM;

    *released = false;

    if ((pairState[pair] != PAIR_FWD || dir <= 0) &&
        (pairState[pair] != PAIR_BWD || dir >= 0)) {
        return RES_OK;
    }

    Result r = solenoidSystem.setPairState(pair_to_index(pair), 0);
    if (r != RES_OK) return r;

    pairState[pair] = PAIR_IDLE;
    *released = true;

    return RES_OK;
}

// ------------------------------------------------------
// Public module factory
// ------------------------------------------------------
const SolenoidBehaviorOps *solenoid_behavior_classic_ops() {
    static const SolenoidBehaviorOps ops = {
        SOL_BEHAVIOR_CLASSIC_PAIRS,
        press,
        release,
    };
    return &ops;
}
