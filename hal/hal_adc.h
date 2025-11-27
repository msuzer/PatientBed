#pragma once
#include <stdint.h>
#include "result.h"

Result hal_adc_read(uint8_t ch, uint16_t *out_value);