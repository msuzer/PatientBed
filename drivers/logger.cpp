#include "logger.h"
#include <Arduino.h>

static LogLevel current_level = LOG_INFO;

// -----------------------------------------
void logger_init(uint32_t baud)
{
    Serial.begin(baud);
    delay(10);
}

void logger_setLevel(LogLevel lvl)
{
    current_level = lvl;
}

LogLevel logger_getLevel()
{
    return current_level;
}

// -----------------------------------------
static void emit(LogLevel lvl, const char *prefix, const char *msg)
{
    if (lvl < current_level) return;
    if (!msg) return;

    Serial.print(prefix);
    Serial.print(msg);
    Serial.print("\r\n");
}

// -----------------------------------------
void log_debug(const char *msg) { emit(LOG_DEBUG, "[DBG] ", msg); }
void log_info (const char *msg) { emit(LOG_INFO,  "[INF] ", msg); }
void log_warn (const char *msg) { emit(LOG_WARN,  "[WRN] ", msg); }
void log_error(const char *msg) { emit(LOG_ERROR, "[ERR] ", msg); }
void log_fatal(const char *msg) { emit(LOG_FATAL, "[FAT] ", msg); }

// -----------------------------------------
void log_info_kv(const char *msg, const char *key, int value)
{
    if (LOG_INFO < current_level) return;
    if (!msg || !key) return;

    Serial.print("[INF] ");
    Serial.print(msg);
    Serial.print(" ");
    Serial.print(key);
    Serial.print("=");

    Serial.print(value);
    Serial.print("\r\n");
}
