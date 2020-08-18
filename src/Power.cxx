#include "Power.hxx"

#include <esp_sleep.h>

#include <cstdint>

#include "Audio.hxx"
#include "Gpio.hxx"
#include "config.h"

void Power::deepSleep() {
    prepareSleep();

    esp_sleep_enable_ext1_wakeup(static_cast<uint64_t>(1) << (PIN_WAKEUP - GPIO_NUM_32 + 32), ESP_EXT1_WAKEUP_ANY_HIGH);
    esp_deep_sleep_start();
}

void Power::prepareSleep() {
    Audio::prepareSleep();
    Gpio::disableAmp();
}

bool Power::isResumeFromSleep() { return esp_sleep_get_wakeup_cause() != ESP_SLEEP_WAKEUP_UNDEFINED; }
