#pragma once
#include <stdint.h>
#include <stdbool.h>
#include "result.h"

enum BuzzerPattern {
    BUZ_PAT_OK = 0,        // "."
    BUZ_PAT_ERROR,         // ".-.-"
    BUZ_PAT_WARN,          // "-.-"
    BUZ_PAT_FATAL,         // "---"
    BUZ_PAT_CLICK,         // "."
    BUZ_PAT_DOUBLE,        // ".."
    BUZ_PAT_TRIPLE,        // "..."
    BUZ_PAT_CUSTOM         // used when calling playPattern(text)
};

Result buzzer_init(uint8_t pin);

// Play a built-in pattern
Result buzzer_play(uint8_t pattern_id);

// Play pattern string e.g. ".-.-"
Result buzzer_playPattern(const char *pattern);

// Call frequently (1–5 ms)
void buzzer_task();

bool buzzer_isBusy();
