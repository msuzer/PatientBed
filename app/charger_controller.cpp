#include "charger_controller.h"

#include "battery.h"
#include "hal_gpio.h"
#include "logger.h"
#include "pins.h"

namespace {

static const uint16_t BATTERY_NOT_PRESENT_mV = 15000;
static const uint16_t BATTERY_PRESENT_mV = 20000;
static const uint16_t VBAT_CHARGE_ON_mV = 24000;
static const uint16_t VBAT_CHARGE_OFF_mV = 28000;
static const uint16_t VBAT_LOG_SIGNIFICANT_DELTA_mV = 500;

enum BatteryStatus {
    NO_STATUS = 0,
    BAT_STAT_ABSENT,
    BAT_STAT_PRESENT,
    BAT_STAT_BIG_CHANGE,
    BAT_STAT_CHARGE_STARTED,
    BAT_STAT_CHARGE_STOPPED_FULL,
    BAT_STAT_CHARGE_STOPPED_ABSENT,
};

uint16_t lastVbat_mV = 0;
bool chargerOn = false;
bool batteryPresent = false;

uint16_t abs_diff_u16(uint16_t a, uint16_t b) {
    return (a >= b) ? (a - b) : (b - a);
}

void battery_log_request(BatteryStatus status, uint16_t vbat_mV) {
    switch (status) {
    case BAT_STAT_ABSENT:
        log_warn(F("Battery removed"));
        break;
    case BAT_STAT_PRESENT:
        log_info(F("Battery connected/present"));
        break;
    case BAT_STAT_BIG_CHANGE:
        break;
    case BAT_STAT_CHARGE_STARTED:
        log_warn(F("Charging started"));
        break;
    case BAT_STAT_CHARGE_STOPPED_FULL:
        log_info(F("Charging stopped (battery full)"));
        break;
    case BAT_STAT_CHARGE_STOPPED_ABSENT:
        log_warn(F("Charging stopped (battery absent)"));
        break;
    default:
        break;
    }

    log_info_kv(F("Vbat"), F("mV"), (int)vbat_mV);
}

void set_charger_output(bool enabled) {
    chargerOn = enabled;
    hal_gpio_write(PIN_CHARGE_RELAY, enabled);
}

} // namespace

Result charger_controller_init() {
    lastVbat_mV = 0;
    chargerOn = false;
    batteryPresent = false;
    hal_gpio_write(PIN_CHARGE_RELAY, false);
    return RES_OK;
}

Result charger_controller_task() {
    BatteryStatus battStatus = NO_STATUS;

    uint16_t vbat_mV = 0;
    Result result = battery_getMillivolts(&vbat_mV);
    if (result != RES_OK) {
        return result;
    }

    if (abs_diff_u16(vbat_mV, lastVbat_mV) >= VBAT_LOG_SIGNIFICANT_DELTA_mV) {
        battStatus = BAT_STAT_BIG_CHANGE;
    }

    if (batteryPresent && vbat_mV < BATTERY_NOT_PRESENT_mV) {
        batteryPresent = false;
        battStatus = BAT_STAT_ABSENT;
    } else if (!batteryPresent && vbat_mV >= BATTERY_PRESENT_mV) {
        batteryPresent = true;
        battStatus = BAT_STAT_PRESENT;
    }

    if (batteryPresent) {
        if (!chargerOn && vbat_mV <= VBAT_CHARGE_ON_mV) {
            set_charger_output(true);
            battStatus = BAT_STAT_CHARGE_STARTED;
        } else if (chargerOn && vbat_mV >= VBAT_CHARGE_OFF_mV) {
            set_charger_output(false);
            battStatus = BAT_STAT_CHARGE_STOPPED_FULL;
        }
    }

    if (!batteryPresent && chargerOn) {
        set_charger_output(false);
        battStatus = BAT_STAT_CHARGE_STOPPED_ABSENT;
    }

    lastVbat_mV = vbat_mV;
    if (battStatus != NO_STATUS) {
        battery_log_request(battStatus, vbat_mV);
    }

    return RES_OK;
}

bool charger_controller_isCharging() {
    return chargerOn;
}
