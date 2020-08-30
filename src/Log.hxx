#ifndef LOG_HXX
#define LOG_HXX

#include <esp_log.h>

#include "config.h"

#define ADABOX_LOG_FORMAT(letter, tag) "[" #letter "] [" tag "] (%d) "

#define LOG_LEVEL_NONE 0
#define LOG_LEVEL_ERROR 1
#define LOG_LEVEL_WARN 2
#define LOG_LEVEL_INFO 3
#define LOG_LEVEL_DEBUG 4
#define LOG_LEVEL_VERBOSE 5

static_assert(LOG_LEVEL_NONE == ESP_LOG_NONE, "log level definition mismatch");
static_assert(LOG_LEVEL_WARN == ESP_LOG_WARN, "log level definition mismatch");
static_assert(LOG_LEVEL_INFO == ESP_LOG_INFO, "log level definition mismatch");
static_assert(LOG_LEVEL_DEBUG == ESP_LOG_DEBUG, "log level definition mismatch");
static_assert(LOG_LEVEL_VERBOSE == ESP_LOG_VERBOSE, "log level definition mismatch");

#if LOG_LEVEL >= LOG_LEVEL_ERROR
#define LOG_ERROR(tag, format, ...) \
    esp_log_write(ESP_LOG_ERROR, tag, ADABOX_LOG_FORMAT(E, tag) format, esp_log_timestamp(), ##__VA_ARGS__)
#else
#define LOG_ERROR(tag, format, ...)
#endif

#if LOG_LEVEL >= LOG_LEVEL_WARN
#define LOG_WARN(tag, format, ...) \
    esp_log_write(ESP_LOG_WARN, tag, ADABOX_LOG_FORMAT(W, tag) format, esp_log_timestamp(), ##__VA_ARGS__)
#else
#define LOG_WARN(tag, format, ...)
#endif

#if LOG_LEVEL >= LOG_LEVEL_INFO
#define LOG_INFO(tag, format, ...) \
    esp_log_write(ESP_LOG_INFO, tag, ADABOX_LOG_FORMAT(I, tag) format, esp_log_timestamp(), ##__VA_ARGS__)
#else
#define LOG_INFO(tag, format, ...)
#endif

#if LOG_LEVEL >= LOG_LEVEL_DEBUG
#define LOG_DEBUG(tag, format, ...) \
    esp_log_write(ESP_LOG_DEBUG, tag, ADABOX_LOG_FORMAT(D, tag) format, esp_log_timestamp(), ##__VA_ARGS__)
#else
#define LOG_DEBUG(tag, format, ...)
#endif

#if LOG_LEVEL >= LOG_LEVEL_VERBOSE
#define LOG_VERBOSE(tag, format, ...) \
    esp_log_write(ESP_LOG_VERBOSE, tag, ADABOX_LOG_FORMAT(V, tag) format, esp_log_timestamp(), ##__VA_ARGS__)
#else
#define LOG_VERBOSE(tag, format, ...)
#endif

namespace Log {

void initialize();

void enableSD();

}  // namespace Log

#endif  // LOG_HXX
