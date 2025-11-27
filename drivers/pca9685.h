#pragma once
#include <stdint.h>
#include <stdbool.h>
#include "result.h"

// PCA9685 Default I2C Address
#define PCA9685_ADDR 0x40

// Register definitions
#define PCA9685_MODE1      0x00
#define PCA9685_MODE2      0x01
#define PCA9685_SUBADR1    0x02
#define PCA9685_SUBADR2    0x03
#define PCA9685_SUBADR3    0x04
#define PCA9685_PRESCALE   0xFE

// LED registers start at 0x06
#define PCA9685_LED0_ON_L  0x06

// MODE1 bits
#define MODE1_SLEEP 0x10
#define MODE1_AI    0x20  // Auto-Increment

// MODE2 bits
#define MODE2_OUTDRV 0x04 // Totem pole (good for solenoids)

// Public API
Result pca9685_init(uint8_t addr = PCA9685_ADDR);
Result pca9685_setPWMFreq(float freq);
Result pca9685_setChannelRaw(uint8_t channel, uint16_t on, uint16_t off);
Result pca9685_setChannelPWM(uint8_t channel, uint16_t duty); // 0–4095
Result pca9685_setChannelState(uint8_t channel, bool on);
