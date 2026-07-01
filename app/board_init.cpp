#include "board_init.h"

#include <Arduino.h>
#include <avr/interrupt.h>

#include "hal_gpio.h"
#include "pins.h"

namespace {

volatile bool tick10msDue = false;

} // namespace

void board_init_gpio() {
    hal_gpio_mode(PIN_MAIN_PUMP_SOLENOID, OUTPUT);
    hal_gpio_write(PIN_MAIN_PUMP_SOLENOID, LOW);

    hal_gpio_mode(PIN_CHARGE_RELAY, OUTPUT);
    hal_gpio_write(PIN_CHARGE_RELAY, LOW);

    hal_gpio_mode(PIN_PCA_OE, OUTPUT);
    hal_gpio_write(PIN_PCA_OE, HIGH);

    hal_gpio_mode(PIN_LED_STATUS, OUTPUT);
    hal_gpio_write(PIN_LED_STATUS, LOW);

    hal_gpio_mode(PIN_BUZZER, OUTPUT);
    hal_gpio_write(PIN_BUZZER, LOW);
}

void board_init_timer10ms() {
    // CTC mode, prescaler 64 -> tick = 4 us; 10 ms / 4 us = 2500 -> OCR1A = 2499
    cli();
    TCCR1A = 0;
    TCCR1B = 0;
    TCCR1B |= (1 << WGM12);
    OCR1A = 2499;
    TCNT1 = 0;
    TIFR1 |= (1 << OCF1A);
    TIMSK1 |= (1 << OCIE1A);
    TCCR1B |= (1 << CS11) | (1 << CS10);
    sei();
}

bool board_consume_tick10ms() {
    if (!tick10msDue) {
        return false;
    }

    tick10msDue = false;
    return true;
}

void board_enable_runtime_outputs() {
    hal_gpio_write(PIN_PCA_OE, LOW);
}

ISR(TIMER1_COMPA_vect) {
    tick10msDue = true;
}
