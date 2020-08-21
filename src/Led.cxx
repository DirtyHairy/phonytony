#include "Led.hxx"

// clang-format off
#include <freertos/FreeRTOS.h>
// clang-format on

#include <Arduino.h>
#include <freertos/task.h>

#include <cmath>

#include "Audio.hxx"
#include "Gpio.hxx"
#include "config.h"

#define PWM_CHANNEL 0
#define PWM_FREQ 10000
#define PWM_RESOLUTION 8

namespace {

uint32_t sampleCount;
uint8_t* samples;
uint8_t sampleIndex;

bool wasPaused;

void _ledTask() {
    Gpio::switchLed(true);
    sampleIndex = 0;
    wasPaused = Audio::isPaused();

    while (true) {
        bool isPaused = Audio::isPaused();

        if (wasPaused != isPaused && isPaused) sampleIndex = 0;
        wasPaused = isPaused;

        ledcWrite(PWM_CHANNEL, isPaused ? samples[sampleIndex] : 0);

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
        samples[i] = 255 - floor(pow((cos(2. * PI * i / static_cast<double>(sampleCount)) + 1.) / 2., 0.75) * 255.);
}

void Led::start() {
    TaskHandle_t ledTaskHandle;
    xTaskCreatePinnedToCore(ledTask, "led", STACK_SIZE_LED, NULL, TASK_PRIORITY_LED, &ledTaskHandle, SERVICE_CORE);
}

void Led::disable() { Gpio::switchLed(false); }
