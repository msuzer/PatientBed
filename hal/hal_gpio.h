#pragma once
#include <stdint.h>
#include <stdbool.h>
#include "result.h"

Result hal_gpio_mode(uint8_t pin, uint8_t mode);
Result hal_gpio_write(uint8_t pin, bool level);
Result hal_gpio_read(uint8_t pin, bool* value_out);
Result hal_gpio_toggle(uint8_t pin);
