#include "Watchdog.hxx"

// clang-format off
#include <freertos/FreeRTOS.h>
// clang-format on

#include <Arduino.h>
#include <freertos/queue.h>
#include <freertos/semphr.h>
#include <freertos/task.h>

#include "Lock.hxx"
#include "Log.hxx"
#include "Power.hxx"
#include "config.h"

#define TAG "watchdog"

namespace {

uint64_t lastNotification;
SemaphoreHandle_t mutex;

void _watchdogTask() {
    while (true) {
        uint64_t delta;
        const uint64_t timestamp = esp_timer_get_time();

        {
            Lock lock(mutex);
            delta = timestamp > lastNotification ? (timestamp - lastNotification) / 1000 : 0;
        }

        if (delta >= WATCHDOG_TIMEOUT_SECONDS * 1000) {
            LOG_INFO(TAG, "deep sleep triggered by watchdog");

            Power::deepSleep();

            return;
        }

        delay(WATCHDOG_TIMEOUT_SECONDS * 1000 - delta);
    }
}

void watchdogTask(void*) {
    _watchdogTask();

    vTaskDelete(NULL);
}

}  // namespace

void Watchdog::initialize() {
    mutex = xSemaphoreCreateMutex();
    notify();
}

void Watchdog::start() {
    TaskHandle_t watchdogTaskHandle;
    xTaskCreatePinnedToCore(watchdogTask, "watchdog", STACK_SIZE_WATCHDOG, NULL, TASK_PRIORITY_WATCHDOG,
                            &watchdogTaskHandle, SERVICE_CORE);
}

void Watchdog::notify() {
    Lock lock(mutex);
    lastNotification = esp_timer_get_time();
}
