#include "Power.hxx"

// clang-format off
#include <freertos/FreeRTOS.h>
// clang-format on

#include <Arduino.h>
#include <driver/adc.h>
#include <esp_adc_cal.h>
#include <esp_sleep.h>
#include <freertos/semphr.h>

#include <atomic>
#include <cstdint>

#include "Audio.hxx"
#include "Gpio.hxx"
#include "Led.hxx"
#include "Lock.hxx"
#include "Log.hxx"
#include "Rfid.hxx"
#include "config.h"

#define TAG "power"

namespace {

SemaphoreHandle_t powerOffMutex;
SemaphoreHandle_t batteryStateMutex;

esp_adc_cal_characteristics_t adcChar;
Power::BatteryState batteryState;
RTC_SLOW_ATTR Power::BatteryState persistentBatteryState;
std::atomic<uint32_t> dbgFixedVoltage;

void measureBatteryState();

void initAdc() {
    pinMode(GPIO_NUM_35, INPUT);
    adc1_config_width(ADC_WIDTH_12Bit);
    adc1_config_channel_atten(ADC1_CHANNEL_7, ADC_ATTEN_11db);

    esp_adc_cal_characterize(ADC_UNIT_1, ADC_ATTEN_11db, ADC_WIDTH_12Bit, 1100, &adcChar);

    measureBatteryState();
}

uint32_t measureVoltage() {
    if (dbgFixedVoltage != 0xffff) return dbgFixedVoltage;

    uint32_t acc = 0;

    for (uint8_t i = 0; i < 64; i++) {
        uint32_t value;

        acc += esp_adc_cal_get_voltage(ADC_CHANNEL_7, &adcChar, &value);

        acc += value;
    }

    return acc / 32 - VOLTAGE_CORRECTION_MV;
}

Power::BatteryState::State getState() {
    uint8_t tp5400Status = Gpio::readTP5400Status();

    if (tp5400Status & 0x01) {
        return Power::BatteryState::State::standby;
    } else if (tp5400Status & 0x02) {
        return Power::BatteryState::charging;
    } else {
        return Power::BatteryState::discharging;
    }
}

void measureBatteryState() {
    Power::BatteryState::State state = getState();
    uint32_t voltage = measureVoltage();

    Lock lock(batteryStateMutex);

    batteryState.state = state;
    batteryState.voltage = voltage;

    Power::BatteryState::Level oldLevel;

    do {
        oldLevel = batteryState.level;
        switch (batteryState.level) {
            case Power::BatteryState::Level::full:
                if (voltage <= THRESHOLD_BATTERY_LOW_MV) batteryState.level = Power::BatteryState::Level::low;

                break;

            case Power::BatteryState::Level::low:
                if (voltage <= THRESHOLD_BATTERY_CRITICAL_MV) batteryState.level = Power::BatteryState::Level::critical;
                if (voltage > (THRESHOLD_BATTERY_LOW_MV + BATTERY_LEVEL_HYSTERESIS_MV))
                    batteryState.level = Power::BatteryState::Level::full;

                break;

            case Power::BatteryState::Level::critical:
                if (voltage <= THRESHOLD_BATTERY_POWEROFF_MV) batteryState.level = Power::BatteryState::Level::poweroff;
                if (voltage > (THRESHOLD_BATTERY_CRITICAL_MV + BATTERY_LEVEL_HYSTERESIS_MV))
                    batteryState.level = Power::BatteryState::Level::low;

                break;

            case Power::BatteryState::Level::poweroff:
                if (voltage > (THRESHOLD_BATTERY_POWEROFF_MV + BATTERY_LEVEL_HYSTERESIS_MV))
                    batteryState.level = Power::BatteryState::Level::full;

                break;
        }
    } while (batteryState.level != oldLevel);

    persistentBatteryState = batteryState;
}

void powerTask_() {
    while (true) {
        // It seems the TP5400 reports an incorrect status immediatelly after all peripherials
        // have powered up -> initial wait in order to give the circuit time to stabilize
        delay(POWER_POLL_INTERVAL);

        measureBatteryState();

        {
            Lock lock(batteryStateMutex);

            if (batteryState.level == Power::BatteryState::Level::poweroff &&
                batteryState.state == Power::BatteryState::discharging) {
                LOG_INFO(TAG, "battery low, shutting down");

                Power::deepSleep();
            }
        }
    }
}

void powerTask(void*) {
    powerTask_();
    vTaskDelete(NULL);
}

}  // namespace

void Power::initialize() {
    powerOffMutex = xSemaphoreCreateMutex();
    batteryStateMutex = xSemaphoreCreateMutex();
    dbgFixedVoltage = 0xffff;

    if (isResumeFromSleep()) batteryState = persistentBatteryState;

    initAdc();
}

void Power::start() {
    TaskHandle_t powerTaskHandle;

    xTaskCreatePinnedToCore(powerTask, "power", STACK_SIZE_POWER, NULL, TASK_PRIORITY_POWER, &powerTaskHandle,
                            SERVICE_CORE);
}

void Power::deepSleep() {
    Audio::stop();
    Led::stop();
    Rfid::stop();
    Gpio::prepareSleep();

    Lock lock(powerOffMutex);

    esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH, ESP_PD_OPTION_OFF);
    esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_FAST_MEM, ESP_PD_OPTION_OFF);
    esp_sleep_pd_config(ESP_PD_DOMAIN_XTAL, ESP_PD_OPTION_OFF);
    esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_SLOW_MEM, ESP_PD_OPTION_ON);

    esp_sleep_enable_ext1_wakeup(static_cast<uint64_t>(1) << (PIN_WAKEUP - GPIO_NUM_32 + 32), ESP_EXT1_WAKEUP_ANY_HIGH);
    esp_deep_sleep_start();
}

bool Power::isResumeFromSleep() { return esp_sleep_get_wakeup_cause() != ESP_SLEEP_WAKEUP_UNDEFINED; }

Power::BatteryState Power::getBatteryState() {
    Lock lock(batteryStateMutex);

    BatteryState _batteryState = batteryState;

    return _batteryState;
}

void Power::dbgSetVoltage(uint32_t voltage) { dbgFixedVoltage = voltage; }
