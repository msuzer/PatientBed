#pragma once

#include <stdint.h>

typedef void (*SerialConsoleKeyHandler)(uint8_t keyId, bool pressed);
typedef bool (*SerialConsoleKeyQuery)(uint8_t keyId);

void serial_console_init(SerialConsoleKeyHandler keyHandler, SerialConsoleKeyQuery isBoundKey);
void serial_console_task();
