#include "keypad.h"
#include "hal_gpio.h"

// ------------------------------------------------------
// Public API
// ------------------------------------------------------
JapaneseKeypad::JapaneseKeypad(const uint8_t *pins)
    : linePins(pins), callback(nullptr), currentRow(0)
{
    memset(keyMatrix, 0, sizeof(keyMatrix));
    memset(lastState, 0, sizeof(lastState));
}

void JapaneseKeypad::begin() {
    for (uint8_t i = 0; i < LINES; i++) {
        releaseLine(i);
    }

    delay(200);  // allow keypad to stabilize

    currentRow = 0;
    driveLineLow(currentRow);
}

void JapaneseKeypad::setCallback(void (*cb)(uint8_t keyIndex, bool pressed)) {
    callback = cb;
}

// ISR-friendly: detection only, no delays, no callbacks
void JapaneseKeypad::tick10ms() {
    // 1) Scan the currently-driven row for event detection
    const uint8_t scannedRow = currentRow;
    uint8_t base = scannedRow * LINES;
    for (uint8_t col = 0; col < LINES; col++) {
        if (col == scannedRow) continue; // skip diagonal (unreachable)
        bool pressed = !sampleLineLevel(col);
        uint8_t idx = base + col;

        if (pressed) {
            if (keyMatrix[idx] < DEBOUNCE_COUNT) keyMatrix[idx]++;
        } else {
            if (keyMatrix[idx] > 0) keyMatrix[idx]--;
        }
    }

    // 2) Release current row back to INPUT_PULLUP
    releaseLine(currentRow);

    // 3) Set next row LOW for the next interrupt cycle
    uint8_t nextRow = currentRow + 1;
    if (nextRow >= LINES) nextRow = 0;
    driveLineLow(nextRow);
    currentRow = nextRow;

    // 4) Edge detection for just-scanned row: queue events for dispatcher
    for (uint8_t col = 0; col < LINES; col++) {
        if (col == scannedRow) continue; // skip diagonal (unreachable)
        uint8_t idx = base + col;
        bool pressedNow = (keyMatrix[idx] == DEBOUNCE_COUNT);
        bool releasedNow = (keyMatrix[idx] == 0);
        bool wasPressed = lastState[idx];

        if (!wasPressed && pressedNow) {
            lastState[idx] = true;
            enqueueEvent(idx, true);
        } else if (wasPressed && releasedNow) {
            lastState[idx] = false;
            enqueueEvent(idx, false);
        }
    }
}

// Main thread: drain queue and call callback
void JapaneseKeypad::dispatch() {
    while (evtTail != evtHead) {
        KeyEvt e = evtQueue[evtTail];
        evtTail = (uint8_t)((evtTail + 1) % EVT_QUEUE_SIZE);
        if (callback) callback(e.id, e.pressed);
    }
}

// ------------------------------------------------------
// Private helpers
// ------------------------------------------------------

bool JapaneseKeypad::sampleLineLevel(uint8_t idx) {
    bool lineLevel = true; // fail safe as "not pressed" if GPIO read fails
    (void)hal_gpio_read(linePins[idx], &lineLevel);
    return lineLevel;  // HIGH = no press, LOW = press
}

void JapaneseKeypad::releaseLine(uint8_t idx) {
    (void)hal_gpio_mode(linePins[idx], INPUT_PULLUP);
}

void JapaneseKeypad::driveLineLow(uint8_t idx) {
    (void)hal_gpio_mode(linePins[idx], OUTPUT);
    (void)hal_gpio_write(linePins[idx], false);
}

bool JapaneseKeypad::getKeyState(uint8_t index) {
    // Return debounced stable state tracked in lastState
    return lastState[index];
}

void JapaneseKeypad::enqueueEvent(uint8_t id, bool pressed) {
    uint8_t nextHead = (uint8_t)((evtHead + 1) % EVT_QUEUE_SIZE);
    if (nextHead == evtTail) {
        // queue full; drop oldest
        evtTail = (uint8_t)((evtTail + 1) % EVT_QUEUE_SIZE);
    }
    evtQueue[evtHead].id = id;
    evtQueue[evtHead].pressed = pressed;
    evtHead = nextHead;
}
