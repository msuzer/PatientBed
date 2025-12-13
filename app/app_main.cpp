#include "app_main.h"
#include <avr/interrupt.h>
#include <stdio.h>  // for snprintf
#include "battery.h"
#include "buzzer.h"
#include "hal_gpio.h"
#include "keypad.h"
#include "logger.h"
#include "pca9685.h"
#include "pins.h"
#include "solenoid.h"

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

// from keypad 2
#define KEY_ID_SOL6_F 49
#define KEY_ID_SOL6_B 94
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

// ---------------------------------------------------------
// Per-pair state (to enforce “first wins” rule)
// ---------------------------------------------------------
enum PairState : uint8_t { PAIR_IDLE = 0, PAIR_FWD, PAIR_BWD };

PairState pairState[9] = {PAIR_IDLE}; // index 1..8 used

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
    logger_setLevel(LOG_INFO);
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

    setup_timer1_10ms();

    // Battery monitor
    if (battery_init(PIN_VBAT_ADC) != RES_OK) {
        log_error(F("battery_init failed"));
        ledMode = LED_MODE_ERROR;
        die_on_fatal_error();
    }

    log_info(F("Battery monitor initialized"));

    // Solenoids (includes PCA9685 + Sol0 setup)
    if (solenoid_init() != RES_OK) {
        log_fatal("solenoid_init failed");
        ledMode = LED_MODE_ERROR;
        die_on_fatal_error();
    }
    
    log_info(F("Solenoid system initialized"));
    
    // Initialize per-pair states
    for (uint8_t p = 1; p <= 8; ++p) {
        pairState[p] = PAIR_IDLE;
    }

    buzzer_play(BUZ_PAT_TRIPLE);
    log_info(F("App init complete"));
}

// =========================================================
// app_loop
// =========================================================
void app_loop() {
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
    // log_info_kv("Key", pressed ? "Press" : "Release", keyIndex);

    int idx = find_binding_for_key(keyIndex);
    if (idx < 0) {
        // Unknown key
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

// Press logic: first key in a pair “wins”
static void handle_binding_press(uint8_t bindIndex) {
    KeyBinding &b = keyBindings[bindIndex];

    if (b.keyId == 0xFF) {
        return; // Placeholder not yet configured
    }

    uint8_t pair = b.pair;
    int8_t dir = b.dir;

    if (pair < 1 || pair > 8) return;

    // Respect first-press-wins rule
    if (pairState[pair] != PAIR_IDLE) {
        log_debug(F("Pair busy"));
        keyAccepted[bindIndex] = false;
        return;
    }

    Result r = solenoid_drive(pair, dir);
    if (r != RES_OK) {
        log_error(F("solenoid_drive failed"));
        ledMode = LED_MODE_ERROR;
        keyAccepted[bindIndex] = false;
        return;
    }

    pairState[pair] = (dir > 0) ? PAIR_FWD : PAIR_BWD;
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

    if (pair < 1 || pair > 8) return;

    // Only stop if this direction is actually active
    if ((pairState[pair] == PAIR_FWD && dir > 0) ||
        (pairState[pair] == PAIR_BWD && dir < 0)) {

        Result r = solenoid_stopPair(pair);
        if (r != RES_OK) {
            log_error(F("solenoid_stopPair failed"));
            ledMode = LED_MODE_ERROR;
            return;
        }

        pairState[pair] = PAIR_IDLE;
        keyAccepted[bindIndex] = false;

        char buffer[16];
        snprintf(buffer, sizeof(buffer), "Sol%d IDLE", pair);
        log_info(buffer);

        buzzer_for_key(pair, dir, false);
    }
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
