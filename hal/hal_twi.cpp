#include "hal_twi.h"
#include <Wire.h>

Result hal_twi_init() {
    Wire.begin(); // assumes SDA=A4, SCL=A5
    return RES_OK;
}

Result hal_twi_write(uint8_t addr, const uint8_t *data, uint8_t len) {
    if (!data || len == 0) return RES_PARAM;

    Wire.beginTransmission(addr);
    Wire.write(data, len);

    uint8_t r = Wire.endTransmission();
    if (r == 0) return RES_OK;   // success
    if (r == 2) return RES_NACK; // no device
    return RES_ERR;
}

Result hal_twi_read(uint8_t addr, uint8_t *data, uint8_t len) {
    if (!data || len == 0) return RES_PARAM;

    uint8_t received = Wire.requestFrom(addr, len);
    if (received != len) return RES_TIMEOUT;

    for (uint8_t i = 0; i < len; i++) {
        if (!Wire.available()) return RES_TIMEOUT;
        data[i] = Wire.read();
    }

    return RES_OK;
}
