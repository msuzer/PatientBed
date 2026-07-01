#include "status_led.h"

#include "hal_gpio.h"
#include "pins.h"

namespace {

static const uint16_t LED_NORMAL_PERIOD_TICKS = 200;
static const uint16_t LED_CHARGING_PERIOD_TICKS = 100;
static const uint16_t LED_ON_TICKS = 10;

StatusLedMode currentMode = StatusLedMode::NORMAL;
uint16_t phaseTick10ms = 0;

} // namespace

void status_led_setMode(StatusLedMode mode) {
    if (currentMode == mode) {
        return;
    }

    currentMode = mode;
    phaseTick10ms = 0;
}

StatusLedMode status_led_getMode() {
    return currentMode;
}

void status_led_tick10ms() {
    if (currentMode == StatusLedMode::ERROR) {
        hal_gpio_write(PIN_LED_STATUS, true);
        return;
    }

    const uint16_t periodTicks =
        (currentMode == StatusLedMode::CHARGING) ? LED_CHARGING_PERIOD_TICKS : LED_NORMAL_PERIOD_TICKS;
    const bool ledOn = (phaseTick10ms < LED_ON_TICKS);

    hal_gpio_write(PIN_LED_STATUS, ledOn);

    phaseTick10ms++;
    if (phaseTick10ms >= periodTicks) {
        phaseTick10ms = 0;
    }
}
