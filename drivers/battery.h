#pragma once
#include <stdint.h>
#include "result.h"

// Call once
Result battery_init(uint8_t adc_channel);

// Call frequently (e.g. every 10–20 ms)
Result battery_task();

// Retrieve filtered voltage
Result battery_getVoltage(float *out_voltage);

// Optional: status helper
bool battery_isLow();

// Configure threshold externally
void battery_setLowThreshold(float volts);
