#include "GpioTask.hxx"

// clang-format off
#include <freertos/FreeRTOS.h>
// clang-format on

#include <MCP23S17.h>
#include <SPI.h>
#include <freertos/queue.h>
#include <freertos/semphr.h>

#include "Lock.hxx"
#include "config.h"
#include "Button.hxx"
#include "AudioTask.hxx"

namespace {

DRAM_ATTR QueueHandle_t gpioInterruptQueue;
SPIClass* spi;
SemaphoreHandle_t spiMutex;

Button buttons[] = {
    Button(
        BTN_PAUSE_MASK,
        []() {
            Serial.println("toggle pause");
            AudioTask::togglePause();
        },
        BTN_POWEROFF_DELAY, []() { Serial.println("power off"); }),
    Button(BTN_VOLUME_DOWN_MASK, BTN_VOLUME_REPEAT,
           []() {
               Serial.println("volume down");
               AudioTask::volumeDown();
           }),
    Button(BTN_VOLUME_UP_MASK, BTN_VOLUME_REPEAT,
           []() {
               Serial.println("volume up");
               AudioTask::volumeUp();
           }),
    Button(
        BTN_PREVIOUS_MASK,
        []() {
            Serial.println("previous");
            AudioTask::previous();
        },
        BTN_REWIND_DELAY,
        []() {
            AudioTask::rewind();
            Serial.println("rewind");
        }),
    Button(BTN_NEXT_MASK, BTN_NEXT_REPEAT,
           []() {
               Serial.println("next");
               AudioTask::next();
           }),
};

extern "C" IRAM_ATTR void gpioIsr() {
    uint32_t val = 1;

    xQueueSendFromISR(gpioInterruptQueue, &val, NULL);
}

void _gpioTask() {
    MCP23S17 mcp23s17(spi, MCP23S17_CS, 0);

    gpioInterruptQueue = xQueueCreate(1, 4);
    pinMode(MCP23S17_IRQ, INPUT_PULLUP);
    attachInterrupt(MCP23S17_IRQ, gpioIsr, FALLING);

    {
        Lock lock(spiMutex);

        mcp23s17.begin();
        mcp23s17.setMirror(false);
        mcp23s17.setInterruptOD(false);
        mcp23s17.setInterruptLevel(LOW);

        for (uint8_t i = 8; i < 13; i++) {
            mcp23s17.pinMode(i, INPUT_PULLUP);
            mcp23s17.enableInterrupt(i, CHANGE);
        }
    }

    while (true) {
        uint64_t timestamp = esp_timer_get_time() / 1000ULL;

        uint32_t timeout = Button::NEVER;
        for (const Button& button : buttons) timeout = std::min(timeout, button.delayToNextNotification(timestamp));

        uint32_t value;
        bool pendingInterrupt = xQueueReceive(gpioInterruptQueue, &value, timeout) == pdTRUE;

        timestamp = esp_timer_get_time() / 1000ULL;

        if (pendingInterrupt) {
            uint8_t pins;

            {
                Lock lock(spiMutex);

                mcp23s17.getInterruptValue();
                pins = mcp23s17.readPort(1);
            }

            for (Button& button : buttons) button.updateState(pins, timestamp);
        }

        for (Button& button : buttons) button.notify(timestamp);
    }
}

void gpioTask(void*) {
    _gpioTask();
    vTaskDelete(NULL);
}

}  // namespace

void GpioTask::initialize(SPIClass& _spi, void* _spiMutex) {
    spi = &_spi;
    spiMutex = _spiMutex;
}

void GpioTask::start() {
    TaskHandle_t gpioTaskHandle;
    xTaskCreatePinnedToCore(gpioTask, "gpio", STACK_SIZE_GPIO, NULL, TASK_PRIORITY_GPIO, &gpioTaskHandle, SERVICE_CORE);
}
