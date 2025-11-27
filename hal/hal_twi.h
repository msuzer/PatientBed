#pragma once
#include <stdint.h>
#include "result.h"

Result hal_twi_init();
Result hal_twi_write(uint8_t addr, const uint8_t* data, uint8_t len);
Result hal_twi_read(uint8_t addr, uint8_t* data, uint8_t len);
