#include "Arduino.h"
#include "keypad.h"
#include "hal_gpio.h"
#include <string.h>

static uint8_t A_pins[KP_ROWS];
static uint8_t B_pins[KP_COLS];

// Internal key states
static bool key_state[KP_KEYS]; // debounced stable state
static bool key_raw[KP_KEYS];   // instantaneous read

// Debounce counters (1 byte per key)
static uint8_t debounce_cnt[KP_KEYS];

// Event queue (ring buffer)
static KeyEvent eventQ[16];
static uint8_t q_head = 0, q_tail = 0;

static bool queue_push(KeyEvent evt)
{
    uint8_t next = (q_head + 1) & 0x0F;
    if (next == q_tail)
        return false; // queue full
    eventQ[q_head] = evt;
    q_head = next;
    return true;
}

static bool queue_pop(KeyEvent *evt)
{
    if (q_tail == q_head)
        return false; // empty
    *evt = eventQ[q_tail];
    q_tail = (q_tail + 1) & 0x0F;
    return true;
}

// -----------------------------------------------------------
// Read a full PASS: drive one side, read the other
// -----------------------------------------------------------
static Result scan_matrix(
    const uint8_t *drive,
    const uint8_t *sense,
    bool pass_id) // 0 or 1
{
    for (uint8_t d = 0; d < KP_ROWS; d++)
    {
        // Set all drive pins to INPUT (Hi-Z)
        for (uint8_t i = 0; i < KP_ROWS; i++)
            hal_gpio_mode(drive[i], INPUT);

        // Activate only drive[d] = LOW
        hal_gpio_mode(drive[d], OUTPUT);
        hal_gpio_write(drive[d], LOW);

        // Small settling delay
        // (for diode/capacitive settling)
        delayMicroseconds(50);

        // Read sense pins
        for (uint8_t s = 0; s < KP_COLS; s++)
        {
            bool val = true;
            Result r = hal_gpio_read(sense[s], &val);
            if (r != RES_OK)
                return r;

            uint8_t key_id = pass_id * 25 + d * 5 + s;
            key_raw[key_id] = (val == false); // active LOW = key pressed
        }
    }

    return RES_OK;
}

// -----------------------------------------------------------
// Initialization
// -----------------------------------------------------------
Result keypad_init(const uint8_t *pinsA, const uint8_t *pinsB)
{
    memcpy(A_pins, pinsA, KP_ROWS);
    memcpy(B_pins, pinsB, KP_COLS);

    // Set everything as input pullup
    for (uint8_t i = 0; i < KP_ROWS; i++)
        hal_gpio_mode(A_pins[i], INPUT_PULLUP);

    for (uint8_t i = 0; i < KP_COLS; i++)
        hal_gpio_mode(B_pins[i], INPUT_PULLUP);

    // Clear states
    memset(key_state, 0, sizeof(key_state));
    memset(key_raw, 0, sizeof(key_raw));
    memset(debounce_cnt, 0, sizeof(debounce_cnt));

    q_head = q_tail = 0;

    return RES_OK;
}

// -----------------------------------------------------------
// Main scanning routine (call 2–5 ms interval)
// -----------------------------------------------------------
Result keypad_task()
{
    // PASS 1 (A → B)
    Result r = scan_matrix(A_pins, B_pins, 0);
    if (r != RES_OK)
        return r;

    // PASS 2 (B → A)
    r = scan_matrix(B_pins, A_pins, 1);
    if (r != RES_OK)
        return r;

    // Debounce and event detection
    for (uint8_t k = 0; k < KP_KEYS; k++)
    {
        bool raw = key_raw[k];
        bool old = key_state[k];

        if (raw != old)
        {
            // in transition — increment counter
            if (debounce_cnt[k] < 5) // 5 samples ≈ 10–25 ms
                debounce_cnt[k]++;
            else
            {
                // stable transition: commit
                key_state[k] = raw;
                debounce_cnt[k] = 0;

                // create event
                KeyEvent evt;
                evt.id = k;
                evt.type = raw ? KEY_EVENT_PRESS : KEY_EVENT_RELEASE;
                queue_push(evt);
            }
        }
        else
        {
            debounce_cnt[k] = 0;
        }
    }

    return RES_OK;
}

// -----------------------------------------------------------
// Retrieve one event (non-blocking)
// -----------------------------------------------------------
bool keypad_getEvent(KeyEvent *evt)
{
    return queue_pop(evt);
}
