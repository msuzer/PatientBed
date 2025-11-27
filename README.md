# PatientBed Control Firmware  
Firmware for ATmega328P-based hydraulic patient bed controller  
using PCA9685, solenoid valves, 5×5 diode matrix keypad,  
battery charging logic, buzzer feedback, and modular HAL/driver architecture.

---

## ✨ Features

### 🔹 Hardware Abstraction Layer (HAL)
- GPIO wrapper (input, output, pullups)
- ADC wrapper (voltage readings, safe abstraction)
- TWI/I2C wrapper (PCA9685 control)

### 🔹 Drivers
- **Solenoid driver**
  - Controls 8 dual-acting hydraulic valves (Forward/Backward)
  - Automatic **Sol0 master valve** handling  
    → Enables main hydraulic flow whenever any solenoid is active  
    → Turns off when all solenoids rest
  - Prevents F/B conflict on each pair
  - Error-aware (returns Result codes)

- **Battery monitor**
  - Reads 24 V battery using 56k/10k divider
  - Voltage filtering (low-pass)
  - Charger relay control with hysteresis (e.g., ON @ 24 V, OFF @ 28 V)

- **Matrix keypad (5×5, diode-separated, Japanese-style)**
  - Dual-direction scanning (A→B, B→A)
  - Debounced events
  - 50 keys potential, but 16 used
  - Event queue (press/release)

- **Buzzer**
  - Non-blocking pattern playback  
  - “Morse-like” patterns (`".-.-"` etc.)  
  - Built-in patterns: OK, ERROR, CLICK, DOUBLE, TRIPLE

- **Logger**
  - Lightweight UART logger  
  - Compile-time per-file tag (`#define LOG_TAG "SOL"`)  
  - printf-free, RAM-free

- **Alive LED**
  - Normal blinking  
  - Fast blink while charging  
  - Solid ON on error
