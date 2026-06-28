#pragma once
#include <stdint.h>
#include "result.h"

// ------------------------------------------------------
// Public API
// ------------------------------------------------------
// Call once
Result battery_init(uint8_t adc_channel);

// Call frequently (e.g. every 10–20 ms)
Result battery_poll();

// Retrieve filtered voltage in millivolts
Result battery_getMillivolts(int16_t *out_mV);
