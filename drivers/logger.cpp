#include "logger.h"
#include <Arduino.h>

static LogLevel current_level = (LogLevel)LOG_DEFAULT_LEVEL;

// -----------------------------------------
void logger_init(uint32_t baud) {
    Serial.begin(baud);
    delay(10);
}

void logger_setLevel(LogLevel lvl) {
    current_level = lvl;
}

LogLevel logger_getLevel() {
    return current_level;
}

// -----------------------------------------
static void emit(LogLevel lvl, const char *prefix, const char *msg) {
    if (lvl < current_level) return;
    if (!msg) return;

    Serial.print(prefix);
    Serial.print(msg);
    Serial.print("\r\n");
}

static void emitFlash(LogLevel lvl, const __FlashStringHelper *prefix, const __FlashStringHelper *msg) {
    if (lvl < current_level) return;
    if (!msg) return;

    Serial.print(prefix);
    Serial.print(msg);
    Serial.print(F("\r\n"));
}

// -----------------------------------------
#if LOG_COMPILE_LEVEL <= LOG_DEBUG
void log_debug(const char *msg) {
    emit(LOG_DEBUG, "[DBG] ", msg);
}
#else
void log_debug(const char *msg) {
    (void)msg;
}
#endif

void log_info(const char *msg) {
    emit(LOG_INFO, "[INF] ", msg);
}
void log_warn(const char *msg) {
    emit(LOG_WARN, "[WRN] ", msg);
}
void log_error(const char *msg) {
    emit(LOG_ERROR, "[ERR] ", msg);
}
void log_fatal(const char *msg) {
    emit(LOG_FATAL, "[FAT] ", msg);
}

#if LOG_COMPILE_LEVEL <= LOG_DEBUG
void log_debug(const __FlashStringHelper *msg) {
    emitFlash(LOG_DEBUG, F("[DBG] "), msg);
}
#else
void log_debug(const __FlashStringHelper *msg) {
    (void)msg;
}
#endif

void log_info(const __FlashStringHelper *msg) {
    emitFlash(LOG_INFO, F("[INF] "), msg);
}
void log_warn(const __FlashStringHelper *msg) {
    emitFlash(LOG_WARN, F("[WRN] "), msg);
}
void log_error(const __FlashStringHelper *msg) {
    emitFlash(LOG_ERROR, F("[ERR] "), msg);
}
void log_fatal(const __FlashStringHelper *msg) {
    emitFlash(LOG_FATAL, F("[FAT] "), msg);
}

// -----------------------------------------
void log_info_kv(const char *msg, const char *key, int value) {
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

void log_info_kv(const __FlashStringHelper *msg, const __FlashStringHelper *key, int value) {
    if (LOG_INFO < current_level) return;
    if (!msg || !key) return;

    Serial.print(F("[INF] "));
    Serial.print(msg);
    Serial.print(F(" "));
    Serial.print(key);
    Serial.print(F("="));
    Serial.print(value);
    Serial.print(F("\r\n"));
}
