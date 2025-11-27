#include "hal_adc.h"
#include <Arduino.h>

Result hal_adc_read(uint8_t ch, uint16_t *out_value) {
    if (!out_value) return RES_PARAM;

    *out_value = analogRead(ch);
    return RES_OK;
}
