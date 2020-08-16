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
        0x01,
        []() {
            Serial.println("toggle pause");
            AudioTask::togglePause();
        },
        3000, []() { Serial.println("power off"); }),
    Button(0x02,
           []() {
               Serial.println("volume down");
               AudioTask::volumeDown();
           }),
    Button(0x04,
           []() {
               Serial.println("volume up");
               AudioTask::volumeUp();
           }),
    Button(0x08, []() { Serial.println("previous"); }),
    Button(0x010, []() { Serial.println("next"); }),
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
        if (xQueueReceive(gpioInterruptQueue, &value, timeout) == pdTRUE) {
            uint8_t pins;

            {
                Lock lock(spiMutex);

                pins = mcp23s17.readPort(1);
                mcp23s17.getInterruptValue();
            }

            for (Button& button : buttons) button.updateState(pins);
        }

        timestamp = esp_timer_get_time() / 1000ULL;

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
