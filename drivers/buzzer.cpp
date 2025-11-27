#include "buzzer.h"
#include "hal_gpio.h"
#include <Arduino.h>
#include <string.h>

// Timing definitions (ms)
static const uint16_t DOT_TIME = 50;
static const uint16_t DASH_TIME = 150;
static const uint16_t GAP_TIME = 50;

static uint8_t buz_pin = 0;
static const char *active_pattern = NULL;
static uint8_t pat_pos = 0;

static bool is_on = false;
static uint32_t stage_end_time = 0;
static bool busy = false;

// Built-in patterns
static const char *patterns[] = {
    ".",    // OK
    ".-.-", // ERROR
    "-.-",  // WARN
    "---",  // FATAL
    ".",    // CLICK
    "..",   // DOUBLE
    "...",  // TRIPLE
    NULL    // CUSTOM placeholder
};

// -------------------------------------------------------
Result buzzer_init(uint8_t pin)
{
    buz_pin = pin;
    hal_gpio_mode(pin, OUTPUT);
    hal_gpio_write(pin, false);
    busy = false;
    active_pattern = NULL;
    return RES_OK;
}

// -------------------------------------------------------
bool buzzer_isBusy()
{
    return busy;
}

// -------------------------------------------------------
static Result buzzer_startPattern(const char *pat)
{
    if (!pat || strlen(pat) == 0)
        return RES_PARAM;

    active_pattern = pat;
    pat_pos = 0;
    busy = true;
    is_on = false;

    // start first element immediately
    stage_end_time = millis();
    return RES_OK;
}

// -------------------------------------------------------
// Play a built-in pattern
// -------------------------------------------------------
Result buzzer_play(uint8_t pattern_id)
{
    if (pattern_id >= sizeof(patterns) / sizeof(patterns[0]))
        return RES_PARAM;

    if (busy)
        return RES_BUSY;

    if (pattern_id == BUZ_PAT_CUSTOM)
        return RES_PARAM;

    return buzzer_startPattern(patterns[pattern_id]);
}

// -------------------------------------------------------
// Play text pattern like ".-.-"
// -------------------------------------------------------
Result buzzer_playPattern(const char *pattern)
{
    if (busy)
        return RES_BUSY;

    return buzzer_startPattern(pattern);
}

// -------------------------------------------------------
// Non-blocking state machine
// -------------------------------------------------------
void buzzer_task()
{
    if (!busy || !active_pattern)
        return;

    uint32_t now = millis();

    if (now < stage_end_time)
        return; // waiting

    // Advance to next stage
    char c = active_pattern[pat_pos];

    if (c == '\0')
    {
        // Finished entire pattern
        hal_gpio_write(buz_pin, false);
        busy = false;
        active_pattern = NULL;
        return;
    }

    if (c == '.' || c == '-')
    {
        // Turn buzzer ON
        is_on = true;
        hal_gpio_write(buz_pin, true);

        uint16_t dur = (c == '.') ? DOT_TIME : DASH_TIME;
        stage_end_time = now + dur;

        // Move to next symbol after ON time elapses
        pat_pos++;
    }
    else if (c == ' ')
    {
        // Explicit silence
        hal_gpio_write(buz_pin, false);
        is_on = false;

        stage_end_time = now + GAP_TIME;
        pat_pos++;
    }
    else
    {
        // Unknown char, skip
        pat_pos++;
        stage_end_time = now; // re-evaluate immediately
    }

    // After ON stage, ensure OFF gap
    if (!is_on)
    {
        hal_gpio_write(buz_pin, false);
    }
}
