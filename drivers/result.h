#pragma once

typedef enum {
    RES_OK = 0,        // Success
    RES_ERR = -1,      // Generic error
    RES_TIMEOUT = -2,  // Timeout waiting for hardware
    RES_NACK = -3,     // I2C device did not acknowledge
    RES_PARAM = -4,    // Invalid parameter
    RES_BUSY = -5,     // Resource busy
    RES_NOOP = -6      // Valid request, no state change performed
} Result;
