#include "Log.hxx"

#include <Arduino.h>
#include <SD.h>

#include <algorithm>

#include "config.h"

#define TAG "log"
#define BUFFER_SIZE 256
#define INITIAL_LOG_BUFFER_SIZE 4096

namespace {

char* initialLogBuffer;
size_t initialLogBufferPos;
File logfile;

int doLog(const char* format, va_list args) {
    char buffer[BUFFER_SIZE];
    int len = vsnprintf(buffer, BUFFER_SIZE, format, args);

    Serial.println(buffer);

#ifdef LOG_TO_SD
    if (logfile) {
        logfile.println(buffer);
        logfile.flush();
    }
#endif

    return len;
}

int doLogPre(const char* format, va_list args) {
    char buffer[BUFFER_SIZE];
    int len = vsnprintf(buffer, BUFFER_SIZE, format, args);

    Serial.println(buffer);

#ifdef LOG_TO_SD
    if (initialLogBufferPos < INITIAL_LOG_BUFFER_SIZE - 1) {
        size_t copyCount = std::min(len, static_cast<int>(INITIAL_LOG_BUFFER_SIZE - initialLogBufferPos - 1));

        strncpy(initialLogBuffer + initialLogBufferPos, buffer, copyCount);

        initialLogBufferPos += (copyCount + 1);
    }
#endif

    return len;
}

}  // namespace

void Log::initialize() {
#ifdef LOG_TO_SD
    initialLogBuffer = (char*)ps_malloc(INITIAL_LOG_BUFFER_SIZE);
    memset(initialLogBuffer, 0, INITIAL_LOG_BUFFER_SIZE);
#endif

    Serial.begin(115200);

    esp_log_level_set("*", static_cast<esp_log_level_t>(LOG_LEVEL));
    esp_log_level_set("wifi", ESP_LOG_ERROR);
    esp_log_set_vprintf(doLogPre);
}

void Log::enableSD() {
#ifndef LOG_TO_SD
    return;
#endif

    logfile = SD.open("/log.txt", FILE_APPEND);

    if (logfile) {
        logfile.println("###############################################################################");

        char* buffer = initialLogBuffer;

        while (buffer - initialLogBuffer < INITIAL_LOG_BUFFER_SIZE && strlen(buffer) > 0) {
            logfile.println(buffer);

            buffer += (strlen(buffer) + 1);
        }

        free(initialLogBuffer);
    } else {
        LOG_ERROR(TAG, "failed to open logfile");
    }

    logfile.flush();

    esp_log_set_vprintf(doLog);
}
