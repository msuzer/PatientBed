#include "serial_console.h"

#include <Arduino.h>
#include <string.h>
#include <stdlib.h>

namespace {

SerialConsoleKeyHandler keyHandler = nullptr;
SerialConsoleKeyQuery isBoundKey = nullptr;

bool parse_key_id(const char *text, uint8_t *outKeyId) {
    if (!text || !outKeyId) {
        return false;
    }

    char *endPtr = nullptr;
    long parsed = strtol(text, &endPtr, 10);
    if (endPtr == text || *endPtr != '\0') {
        return false;
    }
    if (parsed < 0 || parsed > 99) {
        return false;
    }

    *outKeyId = (uint8_t)parsed;
    return true;
}

void handle_line(char *line) {
    if (!line) {
        return;
    }

    char *cmd = strtok(line, " \t");
    if (!cmd) {
        return;
    }

    for (char *p = cmd; *p != '\0'; ++p) {
        if (*p >= 'a' && *p <= 'z') {
            *p = (char)(*p - ('a' - 'A'));
        }
    }

    if (strcmp(cmd, "PING") == 0) {
        Serial.println(F("[SER] PONG"));
        return;
    }

    if (strcmp(cmd, "HELP") == 0) {
        Serial.println(F("[SER] Commands:"));
        Serial.println(F("[SER]   PRESS <id>   (or P <id>)"));
        Serial.println(F("[SER]   RELEASE <id> (or R <id>)"));
        Serial.println(F("[SER]   PING"));
        return;
    }

    const bool isPress = (strcmp(cmd, "PRESS") == 0 || strcmp(cmd, "P") == 0);
    const bool isRelease = (strcmp(cmd, "RELEASE") == 0 || strcmp(cmd, "R") == 0);
    if (!isPress && !isRelease) {
        Serial.print(F("[SER] ERR unknown command: "));
        Serial.println(cmd);
        return;
    }

    char *arg = strtok(nullptr, " \t");
    uint8_t keyId = 0;
    if (!parse_key_id(arg, &keyId)) {
        Serial.println(F("[SER] ERR key id must be 0..99"));
        return;
    }

    if (keyHandler) {
        keyHandler(keyId, isPress);
    }

    Serial.print(F("[SER] OK "));
    Serial.print(isPress ? F("PRESS ") : F("RELEASE "));
    Serial.print(keyId);
    if (isBoundKey && !isBoundKey(keyId)) {
        Serial.print(F(" (unbound)"));
    }
    Serial.print(F("\r\n"));
}

} // namespace

void serial_console_init(SerialConsoleKeyHandler handler, SerialConsoleKeyQuery query) {
    keyHandler = handler;
    isBoundKey = query;
    Serial.println(F("[SER] Ready. Commands: PRESS <id>, RELEASE <id>, P <id>, R <id>, PING, HELP"));
}

void serial_console_task() {
    static char lineBuf[40];
    static uint8_t lineLen = 0;

    while (Serial.available() > 0) {
        char ch = (char)Serial.read();

        if (ch == '\r' || ch == '\n') {
            if (lineLen > 0) {
                lineBuf[lineLen] = '\0';
                handle_line(lineBuf);
                lineLen = 0;
            }
            continue;
        }

        if (ch == '\b' || ch == 127) {
            if (lineLen > 0) {
                lineLen--;
            }
            continue;
        }

        if (ch < 32 || ch > 126) {
            continue;
        }

        if (lineLen < (sizeof(lineBuf) - 1)) {
            lineBuf[lineLen++] = ch;
        } else {
            lineLen = 0;
            Serial.println(F("[SER] ERR line too long"));
        }
    }
}
