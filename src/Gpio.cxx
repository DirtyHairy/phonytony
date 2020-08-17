#include "Gpio.hxx"

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
#include "Audio.hxx"
#include "Power.hxx"

#define DELAY_BEFORE_SLEEP 100

namespace {

DRAM_ATTR QueueHandle_t gpioInterruptQueue;
SPIClass* spi;
SemaphoreHandle_t spiMutex;
MCP23S17* mcp23s17;

Button buttons[] = {
    Button(
        BTN_PAUSE_MASK,
        []() {
            Serial.println("toggle pause");
            Audio::togglePause();
        },
        BTN_POWEROFF_DELAY,
        []() {
            Serial.println("prepare sleep");
            Power::prepareSleep();
        },
        []() {
            delay(DELAY_BEFORE_SLEEP);
            Serial.println("sleep");
            Power::deepSleep();
        }),
    Button(BTN_VOLUME_DOWN_MASK, BTN_VOLUME_REPEAT,
           []() {
               Serial.println("volume down");
               Audio::volumeDown();
           }),
    Button(BTN_VOLUME_UP_MASK, BTN_VOLUME_REPEAT,
           []() {
               Serial.println("volume up");
               Audio::volumeUp();
           }),
    Button(
        BTN_PREVIOUS_MASK,
        []() {
            Serial.println("previous");
            Audio::previous();
        },
        BTN_REWIND_DELAY,
        []() {
            Audio::rewind();
            Serial.println("rewind");
        }),
    Button(BTN_NEXT_MASK, BTN_NEXT_REPEAT,
           []() {
               Serial.println("next");
               Audio::next();
           }),
};

extern "C" IRAM_ATTR void gpioIsr() {
    uint32_t val = 1;

    xQueueSendFromISR(gpioInterruptQueue, &val, NULL);
}

void _gpioTask() {
    attachInterrupt(PIN_MCP23S17_IRQ, gpioIsr, FALLING);

    {
        Lock lock(spiMutex);

        for (uint8_t i = 8; i < 13; i++) mcp23s17->enableInterrupt(i, CHANGE);
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

                mcp23s17->getInterruptValue();
                pins = mcp23s17->readPort(1);
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

void Gpio::initialize(SPIClass& _spi, void* _spiMutex) {
    spi = &_spi;
    spiMutex = _spiMutex;

    mcp23s17 = new MCP23S17(spi, PIN_MCP23S17_CS, 0);
    gpioInterruptQueue = xQueueCreate(1, 4);

    pinMode(PIN_MCP23S17_IRQ, INPUT_PULLUP);

    {
        Lock lock(spiMutex);

        mcp23s17->begin();
        mcp23s17->setMirror(false);
        mcp23s17->setInterruptOD(false);
        mcp23s17->setInterruptLevel(LOW);

        mcp23s17->pinMode(PIN_AMP_ENABLE, OUTPUT);
        mcp23s17->digitalWrite(PIN_AMP_ENABLE, LOW);

        for (uint8_t i = 8; i < 13; i++) {
            mcp23s17->pinMode(i, INPUT_PULLUP);
        }
    }
}

void Gpio::start() {
    TaskHandle_t gpioTaskHandle;
    xTaskCreatePinnedToCore(gpioTask, "gpio", STACK_SIZE_GPIO, NULL, TASK_PRIORITY_GPIO, &gpioTaskHandle, SERVICE_CORE);
}

void Gpio::enableAmp() {
    Lock lock(spiMutex);
    mcp23s17->digitalWrite(PIN_AMP_ENABLE, HIGH);
}

void Gpio::disableAmp() {
    Lock lock(spiMutex);
    mcp23s17->digitalWrite(PIN_AMP_ENABLE, LOW);
}
