#include "app_main.h"
#include <Arduino.h>
#include <avr/interrupt.h>
#include <stdlib.h>
#include <stdio.h>  // for snprintf
#include "battery.h"
#include "buzzer.h"
#include "hal_gpio.h"
#include "keypad.h"
#include "logger.h"
#include "pca9685.h"
#include "pins.h"
#include "solenoid.h"
#include "solenoid_behavior.h"

// ---------------------------------------------------------
// Config: battery + charger thresholds
// ---------------------------------------------------------
static const int16_t BATTERY_NOT_PRESENT_mV = 15000; // below this, battery likely absent
static const int16_t BATTERY_PRESENT_mV = 20000; // above this, battery likely present
static const int16_t VBAT_CHARGE_ON_mV = 24000;  // start charging below this
static const int16_t VBAT_CHARGE_OFF_mV = 28000; // stop charging above this
static const int16_t VBAT_LOG_SIGNIFICANT_DELTA_mV = 500; // log if changed >= 0.5V

// ---------------------------------------------------------
// Key → solenoid mapping
// ---------------------------------------------------------
// Each solenoid pair has two keys: forward and backward
// from keypad 1
#define KEY_ID_SOL1_F 93
#define KEY_ID_SOL1_B 39
#define KEY_ID_SOL2_F 83
#define KEY_ID_SOL2_B 38
#define KEY_ID_SOL3_F 73
#define KEY_ID_SOL3_B 37
#define KEY_ID_SOL4_F 63
#define KEY_ID_SOL4_B 36
#define KEY_ID_SOL5_F 53
#define KEY_ID_SOL5_B 35
#define KEY_ID_SOL6_F 43
#define KEY_ID_SOL6_B 34

// from keypad 2
// #define KEY_ID_SOL6_F 49
// #define KEY_ID_SOL6_B 94
#define KEY_ID_SOL7_F 48
#define KEY_ID_SOL7_B 84
#define KEY_ID_SOL8_F 47
#define KEY_ID_SOL8_B 74

struct KeyBinding {
    uint8_t keyId; // keypad event id (0..49)
    uint8_t pair;  // solenoid pair 1..8
    int8_t dir;    // +1 = forward, -1 = backward
};

static KeyBinding keyBindings[16] = {
    {KEY_ID_SOL1_F, 1, +1}, {KEY_ID_SOL1_B, 1, -1}, {KEY_ID_SOL2_F, 2, +1},
    {KEY_ID_SOL2_B, 2, -1}, {KEY_ID_SOL3_F, 3, +1}, {KEY_ID_SOL3_B, 3, -1},
    {KEY_ID_SOL4_F, 4, +1}, {KEY_ID_SOL4_B, 4, -1}, {KEY_ID_SOL5_F, 5, +1},
    {KEY_ID_SOL5_B, 5, -1}, {KEY_ID_SOL6_F, 6, +1}, {KEY_ID_SOL6_B, 6, -1},
    {KEY_ID_SOL7_F, 7, +1}, {KEY_ID_SOL7_B, 7, -1}, {KEY_ID_SOL8_F, 8, +1},
    {KEY_ID_SOL8_B, 8, -1},
};

// ---------------------------------------------------------
// Alive LED modes
// ---------------------------------------------------------
enum LedMode : uint8_t {
    LED_MODE_NORMAL = 0,
    LED_MODE_CHARGING,
    LED_MODE_ERROR
};

static LedMode ledMode = LED_MODE_NORMAL;

enum BatteryStatus {
    NO_STATUS = 0,
    BAT_STAT_ABSENT,
    BAT_STAT_PRESENT,
    BAT_STAT_BIG_CHANGE,
    BAT_STAT_CHARGE_STARTED,
    BAT_STAT_CHARGE_STOPPED_FULL,
    BAT_STAT_CHARGE_STOPPED_ABSENT,
};

// Track which bindings we actually “accepted” on press
static bool keyAccepted[16] = {false};

PairState pairState[SOLENOID_PAIR_COUNT + 1] = {PAIR_IDLE}; // index 1..SOLENOID_PAIR_COUNT used

const uint8_t keypadPins[10] = {PIN_KB_A0, PIN_KB_B4, PIN_KB_A1, PIN_KB_B3, PIN_KB_A2, 
    PIN_KB_B2, PIN_KB_A3, PIN_KB_B1, PIN_KB_A4, PIN_KB_B0};

JapaneseKeypad keypad(keypadPins);

// ---------------------------------------------------------
// Forward declarations
// ---------------------------------------------------------
static void led_task();
static void battery_task();
static void setup_timer1_10ms();
static int find_binding_for_key(uint8_t keyId);
static void serial_task();
static void serial_handle_line(char *line);
static bool parse_key_id(const char *text, uint8_t *outKeyId);
static void handle_binding_press(uint8_t bindIndex);
static void handle_binding_release(uint8_t bindIndex);
static void handle_key_event(uint8_t keyIndex, bool pressed);
static void buzzer_for_key(uint8_t pair, int8_t dir, bool press);
static void battery_log_request(BatteryStatus status, int16_t vbat_mV);

static void die_on_fatal_error() {
    ledMode = LED_MODE_ERROR;
    buzzer_play(BUZ_PAT_FATAL);
    while (1) {
        // halt
        delay(1000);
    }
}

// =========================================================
// app_init
// =========================================================
void app_init() {
    logger_init(115200);
    logger_setLevel((LogLevel)LOG_DEFAULT_LEVEL);
    log_info(F("System boot"));

    // Buzzer
    buzzer_init(PIN_BUZZER);
    log_info(F("Buzzer initialized"));

    // Alive LED
    hal_gpio_mode(PIN_LED_STATUS, OUTPUT);
    hal_gpio_write(PIN_LED_STATUS, false);
    ledMode = LED_MODE_NORMAL;

    // Charger relay
    hal_gpio_mode(PIN_CHARGE_RELAY, OUTPUT);
    hal_gpio_write(PIN_CHARGE_RELAY, false);
    log_info(F("IO initialized"));

    keypad.begin();
    keypad.setCallback(handle_key_event);
    log_info(F("Keypad initialized"));
    Serial.println(F("[SER] Ready. Commands: PRESS <id>, RELEASE <id>, P <id>, R <id>, PING, HELP"));

    setup_timer1_10ms();

    // Battery monitor
    if (battery_init(PIN_VBAT_ADC) != RES_OK) {
        log_error(F("battery_init failed"));
        ledMode = LED_MODE_ERROR;
        die_on_fatal_error();
    }

    log_info(F("Battery monitor initialized"));

    // Solenoids (includes PCA9685 + main pump solenoid setup)
    if (solenoid_init() != RES_OK) {
        log_fatal("solenoid_init failed");
        ledMode = LED_MODE_ERROR;
        die_on_fatal_error();
    }
    
    log_info(F("Solenoid system initialized"));

    if (solenoid_behavior_init(pairState) != RES_OK) {
        log_fatal("solenoid_behavior_init failed");
        ledMode = LED_MODE_ERROR;
        die_on_fatal_error();
    }

    buzzer_play(BUZ_PAT_TRIPLE);
    log_info(F("App init complete"));
}

// =========================================================
// app_loop
// =========================================================
void app_loop() {
    serial_task();
    keypad.dispatch();
    buzzer_task();
    battery_task();
}

// Timer1 Compare Match A ISR: fires every 10 ms
ISR(TIMER1_COMPA_vect) {
    keypad.tick10ms();
    battery_poll();
    led_task();
}

// =========================================================
// Key handling
// =========================================================
static void handle_key_event(uint8_t keyIndex, bool pressed) {
    int idx = find_binding_for_key(keyIndex);
    if (idx < 0) { // Unknown key
        log_info_kv("Unk Key", pressed ? "Press" : "Release", keyIndex);
        return;
    }

    if (pressed) {
        handle_binding_press((uint8_t)idx);
    } else {
        handle_binding_release((uint8_t)idx);
    }
}

static int find_binding_for_key(uint8_t keyId) {
    for (int i = 0; i < 16; ++i) {
        if (keyBindings[i].keyId == keyId) return i;
    }
    return -1;
}

// ----------------------------------------------------------
// Inbound serial keyboard emulation
// ----------------------------------------------------------
static void serial_task() {
    static char lineBuf[40];
    static uint8_t lineLen = 0;

    while (Serial.available() > 0) {
        char ch = (char)Serial.read();

        if (ch == '\r' || ch == '\n') {
            if (lineLen > 0) {
                lineBuf[lineLen] = '\0';
                serial_handle_line(lineBuf);
                lineLen = 0;
            }
            continue;
        }

        if (ch == '\b' || ch == 127) {
            if (lineLen > 0) {
                lineLen--;
            }
            continue;
        }

        if (ch < 32 || ch > 126) {
            continue;
        }

        if (lineLen < (sizeof(lineBuf) - 1)) {
            lineBuf[lineLen++] = ch;
        } else {
            lineLen = 0;
            Serial.println(F("[SER] ERR line too long"));
        }
    }
}

static bool parse_key_id(const char *text, uint8_t *outKeyId) {
    if (!text || !outKeyId) return false;

    char *endPtr = nullptr;
    long parsed = strtol(text, &endPtr, 10);
    if (endPtr == text || *endPtr != '\0') return false;
    if (parsed < 0 || parsed > 99) return false;

    *outKeyId = (uint8_t)parsed;
    return true;
}

static void serial_handle_line(char *line) {
    if (!line) return;

    char *cmd = strtok(line, " \t");
    if (!cmd) return;

    for (char *p = cmd; *p != '\0'; ++p) {
        if (*p >= 'a' && *p <= 'z') {
            *p = (char)(*p - ('a' - 'A'));
        }
    }

    if (strcmp(cmd, "PING") == 0) {
        Serial.println(F("[SER] PONG"));
        return;
    }

    if (strcmp(cmd, "HELP") == 0) {
        Serial.println(F("[SER] Commands:"));
        Serial.println(F("[SER]   PRESS <id>   (or P <id>)"));
        Serial.println(F("[SER]   RELEASE <id> (or R <id>)"));
        Serial.println(F("[SER]   PING"));
        return;
    }

    const bool isPress = (strcmp(cmd, "PRESS") == 0 || strcmp(cmd, "P") == 0);
    const bool isRelease = (strcmp(cmd, "RELEASE") == 0 || strcmp(cmd, "R") == 0);
    if (!isPress && !isRelease) {
        Serial.print(F("[SER] ERR unknown command: "));
        Serial.println(cmd);
        return;
    }

    char *arg = strtok(nullptr, " \t");
    uint8_t keyId = 0;
    if (!parse_key_id(arg, &keyId)) {
        Serial.println(F("[SER] ERR key id must be 0..99"));
        return;
    }

    handle_key_event(keyId, isPress);

    Serial.print(F("[SER] OK "));
    Serial.print(isPress ? F("PRESS ") : F("RELEASE "));
    Serial.print(keyId);
    if (find_binding_for_key(keyId) < 0) {
        Serial.print(F(" (unbound)"));
    }
    Serial.print(F("\r\n"));
}

// Press logic: first key in a pair “wins”
static void handle_binding_press(uint8_t bindIndex) {
    KeyBinding &b = keyBindings[bindIndex];

    if (b.keyId == 0xFF) {
        return; // Placeholder not yet configured
    }

    uint8_t pair = b.pair;
    int8_t dir = b.dir;

    if (pair < 1 || pair > SOLENOID_PAIR_COUNT) return;

    bool activated = false;
    Result r = solenoid_behavior_press(pair, dir, pairState, &activated);
    if (r != RES_OK) {
        log_error(F("solenoid activation failed"));
        ledMode = LED_MODE_ERROR;
        keyAccepted[bindIndex] = false;
        return;
    }

    if (!activated) {
        keyAccepted[bindIndex] = false;
        return;
    }

    keyAccepted[bindIndex] = true;

    char buffer[16];
    snprintf(buffer, sizeof(buffer), "Sol%d %s", pair, (dir > 0) ? "FWD" : "BWD");
    log_info(buffer);

    buzzer_for_key(pair, dir, true);
}

// Release logic: undo only if we accepted this key on press
static void handle_binding_release(uint8_t bindIndex) {
    if (!keyAccepted[bindIndex]) {
        return; // we did not accept this key on press
    }

    KeyBinding &b = keyBindings[bindIndex];
    uint8_t pair = b.pair;
    int8_t dir = b.dir;

    if (pair < 1 || pair > SOLENOID_PAIR_COUNT) return;

    bool released = false;
    Result r = solenoid_behavior_release(pair, dir, pairState, &released);
    if (r != RES_OK) {
        log_error(F("solenoid release failed"));
        ledMode = LED_MODE_ERROR;
        return;
    }

    if (!released) {
        keyAccepted[bindIndex] = false;
        return;
    }

    keyAccepted[bindIndex] = false;

    char buffer[16];
    snprintf(buffer, sizeof(buffer), "Sol%d IDLE", pair);
    log_info(buffer);

    buzzer_for_key(pair, dir, false);
}

// ----------------------------------------------------------
// Buzzer mapping for keys
// ----------------------------------------------------------
static void buzzer_for_key(uint8_t /*pair*/, int8_t dir, bool press) {
    if (buzzer_isBusy()) return;

    if (dir > 0) {
        // Forward side
        if (press) {
            // Two short beeps
            buzzer_play(BUZ_PAT_DOUBLE);
        } else {
            buzzer_play(BUZ_PAT_CLICK);
        }
    } else {
        // Backward side
        if (press) {
            // Dot-dash to distinguish reverse
            buzzer_play(BUZ_PAT_DOT_DASH);
        } else {
            buzzer_play(BUZ_PAT_CLICK);
        }
    }
}

// =========================================================
// Charger control
// =========================================================
static void battery_task() {
    static int16_t lastVbat_mV = 0;
    static bool chargerOn = false;
    static bool batteryPresent = false;

    enum BatteryStatus battStatus = NO_STATUS;

    int16_t vbat_mV = 0;
    if (battery_getMillivolts(&vbat_mV) != RES_OK) {
        log_error(F("battery_getMillivolts failed"));
        return;
    }

    // Decide significant logging
    bool bigChange = (abs(vbat_mV - lastVbat_mV) >= VBAT_LOG_SIGNIFICANT_DELTA_mV);
    if (bigChange) {
        battStatus = BAT_STAT_BIG_CHANGE;
    }

    // Battery present/absent transitions
    if (batteryPresent && vbat_mV < BATTERY_NOT_PRESENT_mV) {
        battStatus = BAT_STAT_ABSENT;
        ledMode = LED_MODE_NORMAL;
        batteryPresent = false;
    } else if (!batteryPresent && vbat_mV >= BATTERY_PRESENT_mV) {
        batteryPresent = true;
        battStatus = BAT_STAT_PRESENT;
    }

    // Charger hysteresis (only if battery is present)
    if (batteryPresent) {
        if (!chargerOn && vbat_mV <= VBAT_CHARGE_ON_mV) {
            chargerOn = true;
            hal_gpio_write(PIN_CHARGE_RELAY, true);
            ledMode = LED_MODE_CHARGING;
            battStatus = BAT_STAT_CHARGE_STARTED;
        } else if (chargerOn && vbat_mV >= VBAT_CHARGE_OFF_mV) {
            chargerOn = false;
            hal_gpio_write(PIN_CHARGE_RELAY, false);
            ledMode = LED_MODE_NORMAL;
            battStatus = BAT_STAT_CHARGE_STOPPED_FULL;
        }
    }

    // Safety: if battery is absent at any point, ensure charging is OFF
    if (!batteryPresent && chargerOn) {
        chargerOn = false;
        hal_gpio_write(PIN_CHARGE_RELAY, false);
        ledMode = LED_MODE_NORMAL;
        battStatus = BAT_STAT_CHARGE_STOPPED_ABSENT;
    }

    lastVbat_mV = vbat_mV;
    if (battStatus != NO_STATUS) {
        battery_log_request(battStatus, vbat_mV);
    }
}

static void battery_log_request(BatteryStatus status, int16_t vbat_mV) {
    switch (status) {
    case BAT_STAT_ABSENT:
        log_warn(F("Battery removed"));
        break;
    case BAT_STAT_PRESENT:
        log_info(F("Battery connected/present"));
        break;
    case BAT_STAT_BIG_CHANGE:
        break;
    case BAT_STAT_CHARGE_STARTED:
        log_warn(F("Charging started"));
        break;
    case BAT_STAT_CHARGE_STOPPED_FULL:
        log_info(F("Charging stopped (battery full)"));
        break;
    case BAT_STAT_CHARGE_STOPPED_ABSENT:
        log_warn(F("Charging stopped (battery absent)"));
        break;
    default:
        break;
    }

    log_info_kv(F("Vbat"), F("mV"), (int)vbat_mV);
}

// =========================================================
// Alive LED task
// Normal: slow blink
// Charging: faster blink
// Error: solid ON
// =========================================================
static void led_task() {
    static volatile uint32_t ledTickCount10ms = 0; // incremented in ISR every 10 ms
    
    uint16_t ledIntervalMs = 2000;
    if (ledMode == LED_MODE_CHARGING){
        ledIntervalMs = 1000; // faster blink
    } else if (ledMode == LED_MODE_ERROR) {
        // solid ON, no blinking
        hal_gpio_write(PIN_LED_STATUS, true);
        return;
    }

    if (ledTickCount10ms++ == 0) {
        hal_gpio_write(PIN_LED_STATUS, true);
    } else if (ledTickCount10ms == 10) {
        hal_gpio_write(PIN_LED_STATUS, false);
    } else if (ledTickCount10ms > (ledIntervalMs / 10) ) {
        ledTickCount10ms = 0;
    }
}

// Helper: Configure Timer1 to generate 10 ms interrupts on ATmega328P @16 MHz
static void setup_timer1_10ms() {
    // CTC mode, prescaler 64 → tick = 4 µs; 10 ms / 4 µs = 2500 → OCR1A = 2499
    cli();
    TCCR1A = 0;                 // normal operation
    TCCR1B = 0;                 // stop timer to configure
    TCCR1B |= (1 << WGM12);     // CTC mode (Clear Timer on Compare Match)
    OCR1A = 2499;               // compare value for 10 ms
    TCNT1 = 0;                  // reset counter
    TIFR1 |= (1 << OCF1A);      // clear any pending compare match
    TIMSK1 |= (1 << OCIE1A);    // enable compare A match interrupt
    TCCR1B |= (1 << CS11) | (1 << CS10); // prescaler 64
    sei();
}
