#include "battery.h"
#include "hal_adc.h"
#include <stdlib.h> // for NULL

// ------------------------------------------
// Internal configuration
// ------------------------------------------
static uint8_t _adc_ch = 0;

// Full voltage conversion formula:
//
// Vbatt = raw_adc * (Vref / ADC_max) * ((Rtop + Rbottom) / Rbottom)
//
// For your divider: Rtop = 56k, Rbottom = 10k:
// DividerRatio = 66k / 10k = 6.6
//
// With Vref = 5.0V and ADC_max = 1023:
//
// ADC_TO_VBAT = 5.0f / 1023.0f * 6.6f
//
static const float ADC_TO_VBAT = (5.0f / 1023.0f) * ((56.0f + 10.0f) / 10.0f);

// Filter settings
static const float FILTER_ALPHA = 0.10f; // 10% new, 90% old

// State
static float filtered_vbat = 0.0f;
static bool initialized = false;

// Low battery threshold (default 20V)
static float low_threshold = 20.0f;

// ------------------------------------------
void battery_setLowThreshold(float volts) {
    low_threshold = volts;
}

// ------------------------------------------
bool battery_isLow() {
    return (filtered_vbat < low_threshold);
}

// ------------------------------------------
Result battery_init(uint8_t adc_channel) {
    _adc_ch = adc_channel;
    initialized = false;

    // First reading establishes starting value
    uint16_t raw = 0;
    Result r = hal_adc_read(_adc_ch, &raw);
    if (r != RES_OK) return r;

    filtered_vbat = raw * ADC_TO_VBAT;
    initialized = true;
    return RES_OK;
}

// ------------------------------------------
// Non-blocking periodic update
// Call every ~10ms
// ------------------------------------------
Result battery_task() {
    if (!initialized) return RES_ERR;

    uint16_t raw = 0;
    Result r = hal_adc_read(_adc_ch, &raw);
    if (r != RES_OK) return r;

    float vbat = raw * ADC_TO_VBAT;

    // Low-pass filter to tame noise
    filtered_vbat =
        (FILTER_ALPHA * vbat) + ((1.0f - FILTER_ALPHA) * filtered_vbat);

    return RES_OK;
}

// ------------------------------------------
Result battery_getVoltage(float *out_voltage) {
    if (!initialized || out_voltage == NULL) return RES_PARAM;

    *out_voltage = filtered_vbat;
    return RES_OK;
}
