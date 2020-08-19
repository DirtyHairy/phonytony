#include "Power.hxx"

// clang-format off
#include <freertos/FreeRTOS.h>
// clang-format on

#include <esp_sleep.h>
#include <freertos/semphr.h>

#include <cstdint>

#include "Audio.hxx"
#include "Gpio.hxx"
#include "Lock.hxx"
#include "config.h"

namespace {

SemaphoreHandle_t mutex;

}

void Power::initialize() { mutex = xSemaphoreCreateMutex(); }

void Power::deepSleep() {
    prepareSleep();

    Lock lock(mutex);

    esp_sleep_enable_ext1_wakeup(static_cast<uint64_t>(1) << (PIN_WAKEUP - GPIO_NUM_32 + 32), ESP_EXT1_WAKEUP_ANY_HIGH);
    esp_deep_sleep_start();
}

void Power::prepareSleep() {
    Audio::prepareSleep();
    Gpio::disableAmp();
}

bool Power::isResumeFromSleep() { return esp_sleep_get_wakeup_cause() != ESP_SLEEP_WAKEUP_UNDEFINED; }
