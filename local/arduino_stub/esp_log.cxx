#include "esp_log.h"

#include <Arduino.h>

#include <cstdio>

void esp_log_level_set(const char *, esp_log_level_t) {}

void esp_log_set_vprintf() {}

void esp_log_write(esp_log_level_t level, const char *, const char *format, ...) {
    va_list arg;
    va_start(arg, format);

    vfprintf(stderr, format, arg);
    fprintf(stderr, "\n");
}

uint32_t esp_log_timestamp() { return millis(); }
