#include "Power.hxx"

// clang-format off
#include <freertos/FreeRTOS.h>
// clang-format on

#include <esp_sleep.h>
#include <freertos/semphr.h>

#include <cstdint>

#include "Audio.hxx"
#include "Gpio.hxx"
#include "Led.hxx"
#include "Lock.hxx"
#include "config.h"

#include <Arduino.h>

namespace {

SemaphoreHandle_t mutex;

}

void Power::initialize() { mutex = xSemaphoreCreateMutex(); }

void Power::deepSleep() {
    prepareSleep();

    Lock lock(mutex);

    esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH, ESP_PD_OPTION_OFF);
    esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_FAST_MEM, ESP_PD_OPTION_OFF);
    esp_sleep_pd_config(ESP_PD_DOMAIN_XTAL, ESP_PD_OPTION_OFF);
    esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_SLOW_MEM, ESP_PD_OPTION_ON);

    esp_sleep_enable_ext1_wakeup(static_cast<uint64_t>(1) << (PIN_WAKEUP - GPIO_NUM_32 + 32), ESP_EXT1_WAKEUP_ANY_HIGH);
    esp_deep_sleep_start();
}

void Power::prepareSleep() {
    Audio::prepareSleep();
    Gpio::disableAmp();
    Led::disable();
    Gpio::prepareSleep();
}

bool Power::isResumeFromSleep() { return esp_sleep_get_wakeup_cause() != ESP_SLEEP_WAKEUP_UNDEFINED; }
