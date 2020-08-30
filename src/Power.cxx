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

    for (auto pin :
         {PIN_SD_CS, PIN_I2S_BCK, PIN_I2S_DATA, PIN_I2S_WC, PIN_RFID_CS, PIN_MCP23S17_CS, GPIO_NUM_14, GPIO_NUM_12,
          GPIO_NUM_13, GPIO_NUM_18, GPIO_NUM_19, GPIO_NUM_23, PIN_RFID_IRQ, PIN_MCP23S17_IRQ, PIN_LED, PIN_POWER})
        pinMode(pin, INPUT);

    gpio_deep_sleep_hold_en();

    esp_sleep_enable_ext1_wakeup(static_cast<uint64_t>(1) << (PIN_WAKEUP - GPIO_NUM_32 + 32), ESP_EXT1_WAKEUP_ANY_HIGH);
    esp_deep_sleep_start();
}

void Power::prepareSleep() {
    Audio::prepareSleep();
    Gpio::disableAmp();
    Led::disable();
}

bool Power::isResumeFromSleep() { return esp_sleep_get_wakeup_cause() != ESP_SLEEP_WAKEUP_UNDEFINED; }
