#include "pca9685.h"
#include "hal_twi.h"
#include <Arduino.h>

static uint8_t _i2c_addr = PCA9685_ADDR;

// ---------------------------------------------------------
// Low-level write & read helpers with error propagation
// ---------------------------------------------------------
static Result write8(uint8_t reg, uint8_t val) {
    uint8_t buf[2] = {reg, val};
    return hal_twi_write(_i2c_addr, buf, 2);
}

static Result read8(uint8_t reg, uint8_t *out_val) {
    if (!out_val) return RES_PARAM;

    Result r = hal_twi_write(_i2c_addr, &reg, 1);
    if (r != RES_OK) return r;

    return hal_twi_read(_i2c_addr, out_val, 1);
}

// ---------------------------------------------------------
// Init Sequence
// ---------------------------------------------------------
Result pca9685_init(uint8_t addr) {
    _i2c_addr = addr;

    Result r = hal_twi_init();
    if (r != RES_OK) return r;

    // MODE1 = Auto-Increment enabled
    r = write8(PCA9685_MODE1, MODE1_AI);
    if (r != RES_OK) return r;

    // MODE2 = Totem pole outputs
    r = write8(PCA9685_MODE2, MODE2_OUTDRV);
    if (r != RES_OK) return r;

    delay(10);

    return pca9685_setPWMFreq(1000.0f); // default 1 kHz for solenoids
}

// ---------------------------------------------------------
// Set PWM frequency
// ---------------------------------------------------------
Result pca9685_setPWMFreq(float freq) {
    if (freq < 40 || freq > 1000) return RES_PARAM;

    float prescaleval = 25000000.0;
    prescaleval /= 4096.0;
    prescaleval /= freq;
    prescaleval -= 1.0;

    uint8_t prescale = (uint8_t)(prescaleval + 0.5);

    uint8_t oldmode = 0;
    Result r = read8(PCA9685_MODE1, &oldmode);
    if (r != RES_OK) return r;

    uint8_t sleep = (oldmode & ~MODE1_AI) | MODE1_SLEEP;

    r = write8(PCA9685_MODE1, sleep);
    if (r != RES_OK) return r;

    r = write8(PCA9685_PRESCALE, prescale);
    if (r != RES_OK) return r;

    r = write8(PCA9685_MODE1, oldmode);
    if (r != RES_OK) return r;

    delay(5);

    return write8(PCA9685_MODE1, oldmode | MODE1_AI);
}

// ---------------------------------------------------------
// Write raw ON/OFF values
// ---------------------------------------------------------
Result pca9685_setChannelRaw(uint8_t channel, uint16_t on, uint16_t off) {
    if (channel > 15) return RES_PARAM;

    uint8_t reg = PCA9685_LED0_ON_L + 4 * channel;

    uint8_t buf[5] = {reg, (uint8_t)(on & 0xFF), (uint8_t)(on >> 8),
                      (uint8_t)(off & 0xFF), (uint8_t)(off >> 8)};

    return hal_twi_write(_i2c_addr, buf, 5);
}

// ---------------------------------------------------------
// Duty-cycle friendly wrapper
// ---------------------------------------------------------
Result pca9685_setChannelPWM(uint8_t channel, uint16_t duty) {
    if (duty == 0) return pca9685_setChannelRaw(channel, 0, 4096);

    if (duty >= 4095) return pca9685_setChannelRaw(channel, 4096, 0);

    return pca9685_setChannelRaw(channel, 0, duty);
}

// ---------------------------------------------------------
// Solenoid-friendly ON/OFF
// ---------------------------------------------------------
Result pca9685_setChannelState(uint8_t channel, bool on) {
    return pca9685_setChannelPWM(channel, on ? 4095 : 0);
}

Result pca9685_setAllPinsOff() {
    // Use ALL_LED registers: ON=0, OFF full-off bit set.
    uint8_t buf[5] = {PCA9685_ALL_LED_ON_L, 0x00, 0x00, 0x00, 0x10};
    return hal_twi_write(_i2c_addr, buf, 5);
}
