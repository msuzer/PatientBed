#pragma once
#include <stdint.h>
#include <stdbool.h>
#include "result.h"

#define KP_ROWS 5
#define KP_COLS 5
#define KP_KEYS (KP_ROWS * KP_COLS * 2)

// Types of events
typedef enum {
    KEY_EVENT_PRESS = 1,
    KEY_EVENT_RELEASE = 2
} KeyEventType;

typedef struct {
    uint8_t id;            // 0..49
    KeyEventType type;     // PRESS / RELEASE
} KeyEvent;

// Public API
Result keypad_init(const uint8_t *pinsA, const uint8_t *pinsB);

Result keypad_task();      // call frequently (every 2–5 ms)
bool   keypad_getEvent(KeyEvent *evt);   // returns true if event available
