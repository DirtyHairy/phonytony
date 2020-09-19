#include "Led.hxx"

// clang-format off
#include <freertos/FreeRTOS.h>
// clang-format on

#include <Arduino.h>
#include <freertos/task.h>

#include <cmath>

#include "Audio.hxx"
#include "Gpio.hxx"
#include "Power.hxx"
#include "config.h"

#define PWM_CHANNEL 0
#define PWM_FREQ 10000
#define PWM_RESOLUTION 8
#define _PI 3.1415926535897932384626433832795f

#ifdef COMMON_ANODE
#define toDutyCycle(x) x
#else
#define toDutyCycle(x) (255 - x)
#endif

namespace {

TaskHandle_t ledTaskHandle;
uint32_t sampleCount;
uint8_t* samples;
uint8_t sampleIndex;
Gpio::LED currentLED = Gpio::LED::none;

bool wasPlaying;

Gpio::LED determineLed() {
    Power::BatteryState batteryState = Power::getBatteryState();

    if (batteryState.state != Power::BatteryState::State::discharging) return Gpio::LED::blue;

    return batteryState.level == Power::BatteryState::Level::full ? Gpio::LED::green : Gpio::LED::red;
}

void _ledTask() {
    sampleIndex = 0;
    wasPlaying = Audio::isPlaying();

    while (true) {
        Gpio::LED newLed = determineLed();
        if (newLed != currentLED) Gpio::enableLed(newLed);
        currentLED = newLed;

        bool isPlaying = Audio::isPlaying();

        if (isPlaying != wasPlaying && !isPlaying) sampleIndex = 0;
        wasPlaying = isPlaying;

        ledcWrite(PWM_CHANNEL, isPlaying ? toDutyCycle(255) : samples[sampleIndex]);

        sampleIndex = (sampleIndex + 1) % sampleCount;
        delay(50);
    }
}

void ledTask(void*) {
    _ledTask();
    vTaskDelete(NULL);
}

}  // namespace

void Led::initialize() {
    ledcSetup(PWM_CHANNEL, PWM_FREQ, PWM_RESOLUTION);
    ledcAttachPin(PIN_LED, PWM_CHANNEL);
    ledcWrite(PWM_CHANNEL, 255);

    sampleCount = 60;
    samples = (uint8_t*)malloc(sampleCount);

    for (uint8_t i = 0; i < sampleCount; i++)
        samples[i] = toDutyCycle(
            floorf(powf((cosf(2.f * _PI * i / static_cast<float>(sampleCount)) + 1.f) / 2.f, 0.75f) * 255.f));
}

void Led::start() {
    xTaskCreatePinnedToCore(ledTask, "led", STACK_SIZE_LED, NULL, TASK_PRIORITY_LED, &ledTaskHandle, SERVICE_CORE);
}

void Led::stop() {
    if (ledTaskHandle) vTaskDelete(ledTaskHandle);
    ledTaskHandle = NULL;

    Gpio::enableLed(Gpio::LED::none);
}
