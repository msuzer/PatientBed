#include "hal_gpio.h"
#include <Arduino.h>

Result hal_gpio_mode(uint8_t pin, uint8_t mode) {
    if (pin > 21) return RES_PARAM; // safety
    pinMode(pin, mode);
    return RES_OK;
}

Result hal_gpio_write(uint8_t pin, bool level) {
    if (pin > 21) return RES_PARAM;
    digitalWrite(pin, level ? HIGH : LOW);
    return RES_OK;
}

Result hal_gpio_read(uint8_t pin, bool *value_out) {
    if (!value_out) return RES_PARAM;
    if (pin > 21) return RES_PARAM;

    *value_out = (digitalRead(pin) == HIGH);
    return RES_OK;
}
