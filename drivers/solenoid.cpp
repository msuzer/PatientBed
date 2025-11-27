#include "solenoid.h"
#include "pca9685.h"
#include "hal_gpio.h"
#include "pins.h"   // for SOL0_MAIN (A2)

static bool sol_state[16] = {0};   // tracks PCA9685 outputs
static bool sol0_state = false;    // direct-pin solenoid
static uint8_t active_count = 0;   // how many of Sol1..Sol8 are ON

// ------------------------------------------------------
// Helper: apply Sol0 logic automatically
// ------------------------------------------------------
static Result update_sol0()
{
    bool should_on = (active_count > 0);

    if (should_on == sol0_state)
        return RES_OK; // no change

    sol0_state = should_on;
    return hal_gpio_write(PIN_SOL0_MAIN, should_on);
}

// ------------------------------------------------------
Result solenoid_init()
{
    Result r;

    // Init PCA9685 first
    r = pca9685_init();
    if (r != RES_OK) return r;

    // Init Sol0 pin
    r = hal_gpio_mode(PIN_SOL0_MAIN, OUTPUT);
    if (r != RES_OK) return r;

    r = hal_gpio_write(PIN_SOL0_MAIN, LOW);
    if (r != RES_OK) return r;

    // Clear internal state
    for (int i = 0; i < 16; i++) sol_state[i] = false;
    sol0_state = false;
    active_count = 0;

    return RES_OK;
}

// ------------------------------------------------------
// Set raw channel on/off (with tracking and Sol0 update)
// ------------------------------------------------------
Result solenoid_set(SolenoidChannel ch, bool on)
{
    uint8_t c = (uint8_t)ch;

    bool was_on = sol_state[c];
    if (was_on == on)
        return RES_OK;  // no change needed

    // Pair conflict prevention
    // uint8_t pair = c / 2;
    bool is_forward = ((c % 2) == 0);
    uint8_t other_ch = is_forward ? (c + 1) : (c - 1);

    if (on && sol_state[other_ch])
        return RES_ERR; // invalid: F and B at same time

    // Apply to PCA
    Result r = pca9685_setChannelState(c, on);
    if (r != RES_OK) return r;

    // Track state
    sol_state[c] = on;

    // Update active_count
    if (on) active_count++;
    else    active_count--;

    // Apply automatic Sol0 logic
    return update_sol0();
}

// ------------------------------------------------------
static bool get_pair(uint8_t pair, SolenoidChannel *fwd, SolenoidChannel *bwd)
{
    if (pair < 1 || pair > 8)
        return false;

    uint8_t base = (pair - 1) * 2;
    *fwd = (SolenoidChannel)(base + 0);
    *bwd = (SolenoidChannel)(base + 1);
    return true;
}

// ------------------------------------------------------
Result solenoid_forward(uint8_t pair)
{
    SolenoidChannel f, b;
    if (!get_pair(pair, &f, &b)) return RES_PARAM;

    // Ensure B is off
    Result r = solenoid_set(b, false);
    if (r != RES_OK) return r;

    return solenoid_set(f, true);
}

// ------------------------------------------------------
Result solenoid_backward(uint8_t pair)
{
    SolenoidChannel f, b;
    if (!get_pair(pair, &f, &b)) return RES_PARAM;

    Result r = solenoid_set(f, false);
    if (r != RES_OK) return r;

    return solenoid_set(b, true);
}

// ------------------------------------------------------
Result solenoid_stopPair(uint8_t pair)
{
    SolenoidChannel f, b;
    if (!get_pair(pair, &f, &b)) return RES_PARAM;

    Result r = solenoid_set(f, false);
    if (r != RES_OK) return r;

    return solenoid_set(b, false);
}

// ------------------------------------------------------
// Unified drive command
// direction: +1=forward, -1=backward, 0=stop
// ------------------------------------------------------
Result solenoid_drive(uint8_t pair, int direction)
{
    if (direction > 0)  return solenoid_forward(pair);
    if (direction < 0)  return solenoid_backward(pair);
    return solenoid_stopPair(pair);
}

// ------------------------------------------------------
Result solenoid_allOff()
{
    Result r;
    for (uint8_t c = 0; c < 16; c++)
    {
        if (sol_state[c])
        {
            r = solenoid_set((SolenoidChannel)c, false);
            if (r != RES_OK) return r;
        }
    }
    return RES_OK;
}
