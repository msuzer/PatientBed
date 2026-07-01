#include "app_main.h"
#include <Arduino.h>
#include <stdio.h>  // for snprintf
#include "battery.h"
#include "board_init.h"
#include "buzzer.h"
#include "charger_controller.h"
#include "input_router.h"
#include "keypad.h"
#include "logger.h"
#include "pins.h"
#include "serial_console.h"
#include "status_led.h"
#include "solenoid_behavior.h"
#include "solenoid_system_controller.h"

static SolenoidSystemController solenoidSystem;

const uint8_t keypadPins[] = {PIN_KB_A0, PIN_KB_B4, PIN_KB_A1, PIN_KB_B3, PIN_KB_A2,
    PIN_KB_B2, PIN_KB_A3, PIN_KB_B1, PIN_KB_A4, PIN_KB_B0};

JapaneseKeypad keypad(keypadPins);

// ---------------------------------------------------------
// Forward declarations
// ---------------------------------------------------------
static void enter_fatal_state();
static void handle_key_action_error(bool pressed);
static void handle_key_event(uint8_t keyIndex, bool pressed);
static void handle_accepted_key_event(const InputRouterEvent& event, bool pressed);
static void log_key_action(const InputRouterEvent& event, bool pressed);
static void buzzer_for_key(uint8_t pairIndex, SolenoidPairState state, bool press);

static void enter_fatal_state() {
    status_led_setMode(StatusLedMode::ERROR);
    buzzer_play(BUZ_PAT_FATAL);
    while (1) {
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

    board_init_gpio();
    log_info(F("IO initialized"));

    buzzer_init(PIN_BUZZER);
    log_info(F("Buzzer initialized"));

    status_led_setMode(StatusLedMode::NORMAL);

    keypad.begin();
    keypad.setCallback(handle_key_event);
    log_info(F("Keypad initialized"));
    serial_console_init(handle_key_event, input_router_is_bound_key);

    if (battery_init(PIN_VBAT_ADC) != RES_OK) {
        log_error(F("battery_init failed"));
        enter_fatal_state();
    }

    log_info(F("Battery monitor initialized"));

    if (charger_controller_init() != RES_OK) {
        log_error(F("charger_controller_init failed"));
        enter_fatal_state();
    }

    log_info(F("Charger controller initialized"));

    if (solenoidSystem.begin() != RES_OK) {
        log_fatal("Solenoid system init failed");
        enter_fatal_state();
    }

    log_info(F("Solenoid system initialized"));

    if (solenoid_behavior_init(solenoidSystem) != RES_OK) {
        log_fatal("solenoid_behavior_init failed");
        enter_fatal_state();
    }

    board_init_timer10ms();

    log_info(F("App init complete"));

    // Enable PCA9685 outputs (active low) only after initialization is complete.
    board_enable_runtime_outputs();
}

// =========================================================
// app_loop
// =========================================================
void app_loop() {
    serial_console_task();
    keypad.dispatch();
    buzzer_task();

    if (charger_controller_task() != RES_OK) {
        log_error(F("charger_controller_task failed"));
    } else if (status_led_getMode() != StatusLedMode::ERROR) {
        status_led_setMode(
            charger_controller_isCharging() ? StatusLedMode::CHARGING : StatusLedMode::NORMAL);
    }

    if (board_consume_tick10ms()) {
        keypad.tick10ms();
        battery_poll();
        status_led_tick10ms();
    }
}

// =========================================================
// Key handling
// =========================================================
static void handle_key_event(uint8_t keyIndex, bool pressed) {
    InputRouterEvent event = {};
    Result result = input_router_handle_key_event(keyIndex, pressed, &event);

    if (!event.recognized) {
        log_info_kv("Unk Key", pressed ? "Press" : "Release", keyIndex);
        return;
    }

    if (result == RES_NOOP) {
        return;
    }

    if (result != RES_OK) {
        handle_key_action_error(pressed);
        return;
    }

    if (!event.accepted) {
        return;
    }

    handle_accepted_key_event(event, pressed);
}

static void handle_key_action_error(bool pressed) {
    log_error(pressed ? F("solenoid activation failed") : F("solenoid release failed"));
    status_led_setMode(StatusLedMode::ERROR);
}

static void handle_accepted_key_event(const InputRouterEvent& event, bool pressed) {
    log_key_action(event, pressed);
    buzzer_for_key(event.pairIndex, event.state, pressed);
}

static void log_key_action(const InputRouterEvent& event, bool pressed) {
    char buffer[16];
    if (pressed) {
        snprintf(buffer, sizeof(buffer), "Sol%d %s", event.pairIndex + 1,
            solenoid_pair_state_name(event.state));
    } else {
        snprintf(buffer, sizeof(buffer), "Sol%d IDLE", event.pairIndex + 1);
    }
    log_info(buffer);
}

// ----------------------------------------------------------
// Buzzer mapping for keys
// ----------------------------------------------------------
static void buzzer_for_key(uint8_t /*pairIndex*/, SolenoidPairState state, bool press) {
    if (buzzer_isBusy()) return;

    if (state == SolenoidPairState::FORWARD) {
        if (press) {
            buzzer_play(BUZ_PAT_DOUBLE);
        } else {
            buzzer_play(BUZ_PAT_CLICK);
        }
    } else if (state == SolenoidPairState::BACKWARD) {
        if (press) {
            buzzer_play(BUZ_PAT_DOT_DASH);
        } else {
            buzzer_play(BUZ_PAT_CLICK);
        }
    }
}
