#include "app_main.h"
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
static const float VBAT_CHARGE_ON_V = 24.0f;  // start charging below this
static const float VBAT_CHARGE_OFF_V = 28.0f; // stop charging above this

// ---------------------------------------------------------
// Alive LED modes
// ---------------------------------------------------------
enum LedMode : uint8_t {
    LED_MODE_NORMAL = 0,
    LED_MODE_CHARGING,
    LED_MODE_ERROR
};

// ---------------------------------------------------------
// Key → solenoid mapping
//
// We currently don't know the actual keypad IDs, so
// define placeholders and fill them after you run the
// matrix-mapper tool.
// ---------------------------------------------------------

// TODO: replace these with actual KeyEvent.id values once known
#define KEY_ID_SOL1_F 0xFF
#define KEY_ID_SOL1_B 0xFF
#define KEY_ID_SOL2_F 0xFF
#define KEY_ID_SOL2_B 0xFF
#define KEY_ID_SOL3_F 0xFF
#define KEY_ID_SOL3_B 0xFF
#define KEY_ID_SOL4_F 0xFF
#define KEY_ID_SOL4_B 0xFF
#define KEY_ID_SOL5_F 0xFF
#define KEY_ID_SOL5_B 0xFF
#define KEY_ID_SOL6_F 0xFF
#define KEY_ID_SOL6_B 0xFF
#define KEY_ID_SOL7_F 0xFF
#define KEY_ID_SOL7_B 0xFF
#define KEY_ID_SOL8_F 0xFF
#define KEY_ID_SOL8_B 0xFF

struct KeyBinding {
    uint8_t keyId; // keypad event id (0..49)
    uint8_t pair;  // solenoid pair 1..8
    int8_t dir;    // +1 = forward, -1 = backward
};

static KeyBinding keyBindings[16] = {
    {KEY_ID_SOL1_F, 1, +1}, {KEY_ID_SOL1_B, 1, -1},
    {KEY_ID_SOL2_F, 2, +1}, {KEY_ID_SOL2_B, 2, -1},
    {KEY_ID_SOL3_F, 3, +1}, {KEY_ID_SOL3_B, 3, -1},
    {KEY_ID_SOL4_F, 4, +1}, {KEY_ID_SOL4_B, 4, -1},
    {KEY_ID_SOL5_F, 5, +1}, {KEY_ID_SOL5_B, 5, -1},
    {KEY_ID_SOL6_F, 6, +1}, {KEY_ID_SOL6_B, 6, -1},
    {KEY_ID_SOL7_F, 7, +1}, {KEY_ID_SOL7_B, 7, -1},
    {KEY_ID_SOL8_F, 8, +1}, {KEY_ID_SOL8_B, 8, -1},
};

// Track which bindings we actually “accepted” on press
static bool keyAccepted[16] = {false};

// ---------------------------------------------------------
// Per-pair state (to enforce “first wins” rule)
// ---------------------------------------------------------
enum PairState : uint8_t { PAIR_IDLE = 0, PAIR_FWD, PAIR_BWD };

static PairState pairState[9] = {PAIR_IDLE}; // index 1..8 used

// ---------------------------------------------------------
// Charger + battery state
// ---------------------------------------------------------
static bool chargerOn = false;
static bool haveBatterySample = false;
static float lastVbat = 0.0f;

// ---------------------------------------------------------
// Alive LED state
// ---------------------------------------------------------
static LedMode ledMode = LED_MODE_NORMAL;
static uint32_t ledLastToggle = 0;
static uint16_t ledIntervalMs = 500;
static bool ledState = false;

// ---------------------------------------------------------
// Forward declarations
// ---------------------------------------------------------
static void handle_key_event(const KeyEvent &evt);
static void handle_binding_press(uint8_t bindIndex);
static void handle_binding_release(uint8_t bindIndex);

static int find_binding_for_key(uint8_t keyId);
static void buzzer_for_key(uint8_t pair, int8_t dir, bool press);

static void charger_update(float vbat);
static void led_setMode(LedMode mode);
static void led_task();

// =========================================================
// app_init
// =========================================================
void app_init() {
    logger_init(115200);
    logger_setLevel(LOG_INFO);
    log_info("System boot");

    // Buzzer
    buzzer_init(PIN_BUZZER);

    // Alive LED
    hal_gpio_mode(PIN_LED_STATUS, OUTPUT);
    hal_gpio_write(PIN_LED_STATUS, false);
    led_setMode(LED_MODE_NORMAL);

    // Charger relay
    hal_gpio_mode(PIN_CHARGE_RELAY, OUTPUT);
    hal_gpio_write(PIN_CHARGE_RELAY, false);
    chargerOn = false;

    // Keypad wiring
    const uint8_t A[5] = {PIN_KB_A0, PIN_KB_A1, PIN_KB_A2, PIN_KB_A3, PIN_KB_A4};
    const uint8_t B[5] = {PIN_KB_B0, PIN_KB_B1, PIN_KB_B2, PIN_KB_B3, PIN_KB_B4};
    keypad_init(A, B);

    // Battery monitor
    if (battery_init(PIN_VBAT_ADC) != RES_OK) {
        log_error("battery_init failed");
        led_setMode(LED_MODE_ERROR);
    }

    battery_setLowThreshold(20.5); // for example

    pca9685_init();           // Default address 0x40
    pca9685_setPWMFreq(1000); // 1 kHz, good for solenoids

    // Solenoids (includes PCA9685 + Sol0 setup)
    if (solenoid_init() != RES_OK) {
        log_fatal("solenoid_init failed");
        led_setMode(LED_MODE_ERROR);
    } else {
        log_info("Solenoid system ready");
    }

    // Initialize per-pair states
    for (uint8_t p = 1; p <= 8; ++p) {
        pairState[p] = PAIR_IDLE;
    }

    log_info("App init complete");
}

// =========================================================
// app_loop
// =========================================================
void app_loop() {
    // 1) Periodic subsystem tasks
    keypad_task();
    battery_task();
    buzzer_task();
    led_task();

    // 2) Process keypad events → solenoids + buzzer + log
    KeyEvent evt;
    while (keypad_getEvent(&evt)) {
        handle_key_event(evt);
    }

    // 3) Read battery and update charger state
    float v;
    if (battery_getVoltage(&v) == RES_OK) {
        lastVbat = v;
        haveBatterySample = true;
        charger_update(v);
    }


}

// =========================================================
// Key handling
// =========================================================
static void handle_key_event(const KeyEvent &evt)
{
    int idx = find_binding_for_key(evt.id);
    if (idx < 0) {
        // Unknown key – might be one of the unused 34 “ghost” IDs
        log_debug("Unmapped key event");
        return;
    }

    // const KeyBinding &bind = keyBindings[idx];

    if (evt.type == KEY_EVENT_PRESS) {
        handle_binding_press((uint8_t)idx);
    } else {
        handle_binding_release((uint8_t)idx);
    }
}

static int find_binding_for_key(uint8_t keyId)
{
    for (int i = 0; i < 16; ++i) {
        if (keyBindings[i].keyId == keyId)
            return i;
    }
    return -1;
}

// Press logic: first key in a pair “wins”
static void handle_binding_press(uint8_t bindIndex)
{
    KeyBinding &b = keyBindings[bindIndex];

    if (b.keyId == 0xFF) {
        // Placeholder not yet configured
        log_debug("Pressed key with placeholder ID");
        return;
    }

    uint8_t pair = b.pair;
    int8_t dir   = b.dir;

    if (pair < 1 || pair > 8) return;

    // Respect first-press-wins rule
    if (pairState[pair] != PAIR_IDLE) {
        log_debug("Pair busy, ignoring second direction press");
        keyAccepted[bindIndex] = false;
        return;
    }

    Result r = solenoid_drive(pair, dir);
    if (r != RES_OK) {
        log_error("solenoid_drive failed");
        led_setMode(LED_MODE_ERROR);
        keyAccepted[bindIndex] = false;
        return;
    }

    pairState[pair] = (dir > 0) ? PAIR_FWD : PAIR_BWD;
    keyAccepted[bindIndex] = true;

    log_info_kv("key press", "pair", pair);

    // Buzzer feedback
    buzzer_for_key(pair, dir, true);
}

// Release logic: undo only if we accepted this key on press
static void handle_binding_release(uint8_t bindIndex)
{
    if (!keyAccepted[bindIndex]) {
        // We ignored this key's press earlier; ignore release too.
        return;
    }

    KeyBinding &b = keyBindings[bindIndex];
    uint8_t pair = b.pair;
    int8_t dir   = b.dir;

    if (pair < 1 || pair > 8) return;

    // Only stop if this direction is actually active
    if ((pairState[pair] == PAIR_FWD && dir > 0) ||
        (pairState[pair] == PAIR_BWD && dir < 0)) {

        Result r = solenoid_stopPair(pair);
        if (r != RES_OK) {
            log_error("solenoid_stopPair failed");
            led_setMode(LED_MODE_ERROR);
            return;
        }

        pairState[pair] = PAIR_IDLE;
        keyAccepted[bindIndex] = false;

        log_info_kv("key release", "pair", pair);

        // Buzzer feedback
        buzzer_for_key(pair, dir, false);
    }
}

// ----------------------------------------------------------
// Buzzer mapping for keys
//
// Same patterns across all pairs:
//   Forward press   → e.g. DOUBLE
//   Forward release → CLICK
//   Backward press  → TRIPLE
//   Backward release→ CLICK
// ----------------------------------------------------------
static void buzzer_for_key(uint8_t /*pair*/, int8_t dir, bool press)
{
    if (buzzer_isBusy())
        return;

    if (dir > 0) {
        // Forward side
        if (press) {
            buzzer_play(BUZ_PAT_DOUBLE);
        } else {
            buzzer_play(BUZ_PAT_CLICK);
        }
    } else {
        // Backward side
        if (press) {
            buzzer_play(BUZ_PAT_TRIPLE);
        } else {
            buzzer_play(BUZ_PAT_CLICK);
        }
    }
}

// =========================================================
// Charger control
// =========================================================
static void charger_update(float vbat)
{
    // Hysteresis: ON below 24V, OFF above 28V
    if (!chargerOn && vbat <= VBAT_CHARGE_ON_V) {
        chargerOn = true;
        hal_gpio_write(PIN_CHARGE_RELAY, true);
        led_setMode(LED_MODE_CHARGING);
        log_warn("Charging started");

        // Optional: small beep
        if (!buzzer_isBusy()) {
            buzzer_play(BUZ_PAT_OK);
        }
    }
    else if (chargerOn && vbat >= VBAT_CHARGE_OFF_V) {
        chargerOn = false;
        hal_gpio_write(PIN_CHARGE_RELAY, false);
        led_setMode(LED_MODE_NORMAL);
        log_info("Charging stopped");

        // Optional: small beep (comment out if you prefer silent)
        if (!buzzer_isBusy()) {
            buzzer_play(BUZ_PAT_OK);
        }
    }
}

// =========================================================
// Alive LED task
// Normal: slow blink
// Charging: faster blink
// Error: solid ON
// =========================================================
static void led_setMode(LedMode mode)
{
    if (mode == ledMode)
        return;

    ledMode = mode;

    switch (ledMode) {
    case LED_MODE_NORMAL:
        ledIntervalMs = 500;   // 1 Hz blink
        break;
    case LED_MODE_CHARGING:
        ledIntervalMs = 200;   // faster blink
        break;
    case LED_MODE_ERROR:
        // solid ON, no blinking
        hal_gpio_write(PIN_LED_STATUS, true);
        return;
    }

    // reset timer for blinking modes
    ledLastToggle = millis();
    ledState = false;
    hal_gpio_write(PIN_LED_STATUS, ledState);
}

static void led_task()
{
    if (ledMode == LED_MODE_ERROR) {
        // solid ON, nothing to do
        return;
    }

    uint32_t now = millis();
    if (now - ledLastToggle >= ledIntervalMs) {
        ledLastToggle = now;
        ledState = !ledState;
        hal_gpio_write(PIN_LED_STATUS, ledState);
    }
}