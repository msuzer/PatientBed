#ifndef JAPANESE_KEYPAD_H
#define JAPANESE_KEYPAD_H

#include <Arduino.h>

// ------------------------------------------------------
// Japanese keypad scanner
// ------------------------------------------------------
class JapaneseKeypad {
public:
    static const uint8_t LINES = 10;
    static const uint8_t KEY_COUNT = LINES * LINES;
    // Number of consecutive detections required for a stable press
    static const uint8_t DEBOUNCE_COUNT = 2;

    JapaneseKeypad(const uint8_t *pins);

    void begin();       // Must be called from setup()
    void tick10ms();    // Call from 10ms timer ISR: detection only (no callbacks)
    void dispatch();    // Call from loop(): drain event queue and call callback
    void setCallback(void (*cb)(uint8_t keyIndex, bool pressed));

private:
    // --------------------------------------------------
    // Private state
    // --------------------------------------------------
    const uint8_t *linePins;
    void (*callback)(uint8_t keyIndex, bool pressed);

    uint8_t keyMatrix[KEY_COUNT];
    uint8_t lastState[KEY_COUNT];
    uint8_t currentRow;

    // Simple ring buffer to queue events from ISR to main thread
    struct KeyEvt { uint8_t id; bool pressed; };
    static const uint8_t EVT_QUEUE_SIZE = 16;
    KeyEvt evtQueue[EVT_QUEUE_SIZE];
    volatile uint8_t evtHead = 0; // write index (ISR)
    volatile uint8_t evtTail = 0; // read index (main thread)

    // --------------------------------------------------
    // Private helpers
    // --------------------------------------------------
    // Internal helpers
    void enqueueEvent(uint8_t id, bool pressed);
    void releaseLine(uint8_t idx);
    void driveLineLow(uint8_t idx);
    bool sampleLineLevel(uint8_t idx);
    bool getKeyState(uint8_t index);
};

#endif
