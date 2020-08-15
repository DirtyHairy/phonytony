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

namespace {

DRAM_ATTR QueueHandle_t gpioInterruptQueue;
SPIClass* spi;
SemaphoreHandle_t spiMutex;

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
        uint32_t value;
        if (xQueueReceive(gpioInterruptQueue, &value, 100) != pdTRUE) continue;

        uint16_t signalingPins, pins;

        {
            Lock lock(spiMutex);

            signalingPins = mcp23s17.getInterruptPins();
            pins = mcp23s17.getInterruptValue();
        }

        uint8_t pushedButtons = (signalingPins >> 8) & (pins >> 8);

        if (pushedButtons) Serial.printf("GPIO interrupt: 0x%02x\r\n", pushedButtons);
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
