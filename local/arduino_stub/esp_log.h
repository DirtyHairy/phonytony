#ifndef ESP_LOG_H
#define ESP_LOG_H

#include <stdarg.h>

#include <cstdint>

#define ESP_LOG_NONE 0
#define ESP_LOG_ERROR 1
#define ESP_LOG_WARN 2
#define ESP_LOG_INFO 3
#define ESP_LOG_DEBUG 4
#define ESP_LOG_VERBOSE 5

typedef int esp_log_level_t;

typedef int (*vprintf_like_t)(const char *, va_list);

void esp_log_level_set(const char *, esp_log_level_t);

void esp_log_set_vprintf();

void esp_log_write(esp_log_level_t, const char *, const char *, ...);

uint32_t esp_log_timestamp();

#endif  // ESP_LOG_H
