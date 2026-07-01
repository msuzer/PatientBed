# PatientBed Control Firmware

Firmware for an ATmega328P/ATmega328PB hydraulic patient bed controller.

The system drives solenoid valves through a PCA9685, reads a diode-isolated Japanese matrix keypad, monitors battery/charging state, provides buzzer/LED feedback, and keeps hardware access behind small HAL/driver layers.

## Architecture

```text
src/main.cpp
  -> app/app_main.cpp
      -> keypad event handling
      -> battery/charger task
      -> buzzer/LED feedback
      -> solenoid behavior selection

app/solenoid_behavior.cpp
  -> selects behavior mode at boot
  -> configures pair mappings for the selected mode
  -> dispatches press/release to behavior module

app/solenoid_behavior_*.cpp
  -> behavior policy only
  -> classic, shared-direction, future scaffold

app/solenoid_system_controller.cpp
  -> generic solenoid pair controller
  -> PCA9685 channel state tracking
  -> pair configuration
  -> main pump enable/disable

drivers/
  -> keypad, PCA9685, buzzer, battery, logger

hal/
  -> GPIO, ADC, TWI/I2C wrappers
```

## Solenoid Control

The solenoid system is split into two layers:

- `SolenoidSystemController`: generic low-level pair control. It knows pair mappings, pair states, PCA9685 channels, and main pump state.
- `solenoid_behavior_*`: high-level hydraulic behavior policy. It decides which pair requests are accepted, ignored, or coordinated.

Current behavior modes:

- **Classic pairs**: 8 complementary pairs plus main pump.
- **Shared direction**: 7 mirrored work pairs, 1 complementary direction pair, plus main pump.
- **Future scenario**: scaffold for a later behavior mode.

See [solenoid_behavior_modes.md](solenoid_behavior_modes.md) for current modes and candidate future hydraulic designs.

## Keypad

The keypad driver scans a 10-line diode matrix and emits debounced press/release events.

- Timer ISR calls `keypad.tick10ms()`.
- Main loop calls `keypad.dispatch()`.
- App-level key bindings map keypad event IDs to zero-based solenoid pair indexes and `SolenoidPairState` values.

The debounce is intentionally conservative for human interaction. The keypad hardware is assumed to include diodes.

## Serial Debug Interface

The firmware exposes a simple serial interface for debugging and bench testing. It lets a user emulate keypad events over the serial monitor without physically pressing the keypad.

Supported commands:

```text
PRESS <key_id>
RELEASE <key_id>
P <key_id>
R <key_id>
PING
HELP
```

`PRESS`/`P` and `RELEASE`/`R` route through the same `handle_key_event()` path used by real keypad events, so solenoid behavior, key acceptance, release handling, logging, and buzzer feedback are exercised the same way.

## Feedback

The buzzer is non-blocking and plays short dot/dash-style patterns.

During boot, the selected solenoid behavior mode is announced with a buzzer hint:

- Classic: one dot
- Shared direction: two dots
- Future/scaffold: three dots

The alive LED indicates general status:

- slow blink: normal
- faster blink: charging
- solid on: error/fatal state

## Battery And Charging

The battery driver reads pack voltage through an ADC divider. `app_main` applies charger relay hysteresis and logs meaningful battery state transitions.

## Build

This is a PlatformIO project.

```sh
pio run
```

Configured environments:

- `atmega328p_custom`
- `atmega328pb_custom`

## Important Files

- [app/app_main.cpp](app/app_main.cpp): main application orchestration.
- [app/solenoid_system_controller.h](app/solenoid_system_controller.h): generic solenoid controller API.
- [app/solenoid_behavior.cpp](app/solenoid_behavior.cpp): behavior selection and dispatch.
- [app/solenoid_behavior_shared.cpp](app/solenoid_behavior_shared.cpp): shared-direction policy.
- [app/solenoid_system_config.h](app/solenoid_system_config.h): pair mappings per behavior mode.
- [config/config.h](config/config.h): behavior selector and mode config.
- [config/pins.h](config/pins.h): board pin mapping.
- [solenoid_behavior_modes.md](solenoid_behavior_modes.md): mode reference and future mode notes.
