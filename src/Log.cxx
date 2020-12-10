#include "Log.hxx"

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/semphr.h>
#include <freertos/task.h>

#include <Lock.hxx>
#include <algorithm>
#include <atomic>
#include <cstdio>

#include "config.h"

#define TAG "log"
#define LOGFILE "/sdcard/log.txt"
#define BUFFER_SIZE 256
#define INITIAL_LOG_BUFFER_SIZE 4096
#define LOG_TO_SD_QUEUE_SIZE 4
#define WRITE_TO_SD_SHUTDOWN_GRACE_TIME 500000

namespace {

char* initialLogBuffer = nullptr;
size_t initialLogBufferPos;

SemaphoreHandle_t initialBufferMutex;
SemaphoreHandle_t logBufferMutex;
SemaphoreHandle_t writeToSdMutex;
QueueHandle_t logToSdQueue;
TaskHandle_t logToSdTaskHandle = nullptr;
std::atomic<bool> shutdown;

char logBuffer[BUFFER_SIZE];

int doLog(const char* format, va_list args) {
    Lock bufferLock(logBufferMutex);

    int len = vsnprintf(logBuffer, BUFFER_SIZE, format, args);

    Serial.println(logBuffer);

#ifdef LOG_TO_SD
    if (logToSdTaskHandle) {
        xQueueSend(logToSdQueue, logBuffer, portMAX_DELAY);
    }
#endif

    return len;
}

int doLogPre(const char* format, va_list args) {
    Lock bufferLock(logBufferMutex);

    int len = vsnprintf(logBuffer, BUFFER_SIZE, format, args);

    Serial.println(logBuffer);

#ifdef LOG_TO_SD
    Lock initialBufferLock(initialBufferMutex);
    if (!initialLogBuffer) return len;

    if (initialLogBufferPos < INITIAL_LOG_BUFFER_SIZE - 1) {
        size_t copyCount = std::min(len, static_cast<int>(INITIAL_LOG_BUFFER_SIZE - initialLogBufferPos - 1));

        strncpy(initialLogBuffer + initialLogBufferPos, logBuffer, copyCount);

        initialLogBufferPos += (copyCount + 1);
    }
#endif

    return len;
}

void _logToSdTask() {
    char logEntry[BUFFER_SIZE];

    while (true) {
        xQueueReceive(logToSdQueue, logEntry, portMAX_DELAY);

        Lock lock(writeToSdMutex);
        if (shutdown) return;

        FILE* logfile = fopen(LOGFILE, "a");

        if (logfile) {
            fputs(logEntry, logfile);
            fputs("\r\n", logfile);

            fclose(logfile);
        }
    }
}

void logToSdTask(void*) {
    _logToSdTask();
    vTaskDelete(NULL);
}

}  // namespace

void Log::initialize() {
    shutdown = false;
    logBufferMutex = xSemaphoreCreateMutex();

    Serial.begin(115200);

    esp_log_level_set("*", static_cast<esp_log_level_t>(LOG_LEVEL));
    esp_log_level_set("wifi", ESP_LOG_ERROR);

#ifdef LOG_TO_SD
    initialLogBuffer = (char*)ps_malloc(INITIAL_LOG_BUFFER_SIZE);
    memset(initialLogBuffer, 0, INITIAL_LOG_BUFFER_SIZE);

    initialBufferMutex = xSemaphoreCreateMutex();

    esp_log_set_vprintf(doLogPre);
#else
    esp_log_set_vprintf(doLog);
#endif
}

void Log::enableSD() {
#ifndef LOG_TO_SD
    return;
#endif

    FILE* logfile;

    {
        Lock initialBufferLock(initialBufferMutex);

        logfile = fopen(LOGFILE, "a");

        if (logfile) {
            fputs("###############################################################################\n", logfile);

            char* buffer = initialLogBuffer;

            while (buffer - initialLogBuffer < INITIAL_LOG_BUFFER_SIZE && strlen(buffer) > 0) {
                fputs(buffer, logfile);
                fputs("\r\n", logfile);

                buffer += (strlen(buffer) + 1);
            }

            fclose(logfile);

            writeToSdMutex = xSemaphoreCreateMutex();
            logToSdQueue = xQueueCreate(LOG_TO_SD_QUEUE_SIZE, BUFFER_SIZE);
            xTaskCreatePinnedToCore(logToSdTask, "logToSD", STACK_SIZE_LOG_TO_SD, nullptr, TASK_PRIORITY_LOG_TO_SD,
                                    &logToSdTaskHandle, SERVICE_CORE);
        }

        free(initialLogBuffer);
        initialLogBuffer = nullptr;
    }

    esp_log_set_vprintf(doLog);

    if (!logfile) {
        LOG_ERROR(TAG, "failed to open logfile");
    }
}

void Log::prepareSleep() {
#ifdef LOG_TO_SD
    if (logToSdTaskHandle || uxQueueMessagesWaiting(logToSdQueue) > 0) {
        uint64_t timestamp = esp_timer_get_time();

        while (uxQueueMessagesWaiting(logToSdQueue) > 0 &&
               esp_timer_get_time() - timestamp < WRITE_TO_SD_SHUTDOWN_GRACE_TIME)
            delay(10);
    }

    Lock lock(writeToSdMutex);
    shutdown = true;
#endif
}
