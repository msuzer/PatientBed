#pragma once
#include <stdint.h>
#include <WString.h>

enum LogLevel {
    LOG_SILENT = 100,
    LOG_FATAL  = 60,
    LOG_ERROR  = 40,
    LOG_WARN   = 30,
    LOG_INFO   = 20,
    LOG_DEBUG  = 10
};

// Compile-time threshold. Messages below this level are removed from binary.
#ifndef LOG_COMPILE_LEVEL
#define LOG_COMPILE_LEVEL LOG_DEBUG
#endif

// Suggested runtime default level if the app chooses to use it.
#ifndef LOG_DEFAULT_LEVEL
#define LOG_DEFAULT_LEVEL LOG_INFO
#endif

void logger_init(uint32_t baud);

void logger_setLevel(LogLevel lvl);
LogLevel logger_getLevel();

void log_debug(const char *msg);
void log_info(const char *msg);
void log_warn(const char *msg);
void log_error(const char *msg);
void log_fatal(const char *msg);

// PROGMEM-friendly overloads for flash-resident literals
void log_debug(const __FlashStringHelper *msg);
void log_info(const __FlashStringHelper *msg);
void log_warn(const __FlashStringHelper *msg);
void log_error(const __FlashStringHelper *msg);
void log_fatal(const __FlashStringHelper *msg);

void log_info_kv(const char *msg, const char *key, int value);
void log_info_kv(const __FlashStringHelper *msg, const __FlashStringHelper *key, int value);
