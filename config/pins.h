#pragma once
#include <Arduino.h>

// --------------------------------------------------------
// Keyboard Matrix – A side (PD2–PD6 → D2–D6)
// --------------------------------------------------------
static const uint8_t PIN_KB_A0 = 2;  // PD2
static const uint8_t PIN_KB_A1 = 3;  // PD3
static const uint8_t PIN_KB_A2 = 4;  // PD4
static const uint8_t PIN_KB_A3 = 5;  // PD5
static const uint8_t PIN_KB_A4 = 6;  // PD6
// --------------------------------------------------------
// Keyboard Matrix – B side (PB0–PB4 → D8–D12)
// --------------------------------------------------------
static const uint8_t PIN_KB_B0 = 8;   // PB0
static const uint8_t PIN_KB_B1 = 9;   // PB1
static const uint8_t PIN_KB_B2 = 10;  // PB2
static const uint8_t PIN_KB_B3 = 11;  // PB3
static const uint8_t PIN_KB_B4 = 12;  // PB4

// --------------------------------------------------------
// Buzzer (PD7 → D7)
// --------------------------------------------------------
static const uint8_t PIN_BUZZER = 7;

// --------------------------------------------------------
// Status LED (PB5 → D13)
// --------------------------------------------------------
static const uint8_t PIN_LED_STATUS = 13;

// --------------------------------------------------------
// Analog Inputs / Control Outputs (PC0–PC3 → A0–A3)
// --------------------------------------------------------
static const uint8_t PIN_VBAT_ADC      = A0;  // PC0 – battery monitor
static const uint8_t PIN_CHARGE_RELAY  = A1;  // PC1 – relay via NPN
static const uint8_t PIN_SOL0_MAIN     = A2;  // PC2 – solenoid 0
static const uint8_t PIN_PCA_OE        = A3;  // PC3 – OE (active low)

// --------------------------------------------------------
// I2C Pins (PC4/PC5 → A4/A5)
// --------------------------------------------------------
static const uint8_t PIN_I2C_SDA = A4; // PC4
static const uint8_t PIN_I2C_SCL = A5; // PC5

// --------------------------------------------------------
// UART Pins (PD0/PD1 → D0/D1)
// --------------------------------------------------------
static const uint8_t PIN_UART_RX = 0;  // PD0
static const uint8_t PIN_UART_TX = 1;  // PD1

// --------------------------------------------------------
// PCA9685 Channels (0–15) for Sol1F..Sol8B
// --------------------------------------------------------
static const uint8_t SOL_CH_1F = 0;
static const uint8_t SOL_CH_1B = 1;
static const uint8_t SOL_CH_2F = 2;
static const uint8_t SOL_CH_2B = 3;
static const uint8_t SOL_CH_3F = 4;
static const uint8_t SOL_CH_3B = 5;
static const uint8_t SOL_CH_4F = 6;
static const uint8_t SOL_CH_4B = 7;
static const uint8_t SOL_CH_5F = 8;
static const uint8_t SOL_CH_5B = 9;
static const uint8_t SOL_CH_6F = 10;
static const uint8_t SOL_CH_6B = 11;
static const uint8_t SOL_CH_7F = 12;
static const uint8_t SOL_CH_7B = 13;
static const uint8_t SOL_CH_8F = 14;
static const uint8_t SOL_CH_8B = 15;
