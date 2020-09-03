#include "Gpio.hxx"

// clang-format off
#include <freertos/FreeRTOS.h>
// clang-format on

#include <MCP23S17.h>
#include <SPI.h>
#include <freertos/semphr.h>

#include "Audio.hxx"
#include "Button.hxx"
#include "Lock.hxx"
#include "Log.hxx"
#include "Power.hxx"
#include "config.h"

#define TAG "gpio"
#define DELAY_BEFORE_SLEEP 100

namespace {

DRAM_ATTR TaskHandle_t gpioTaskHandle;
SPIClass* spi;
SemaphoreHandle_t spiMutex;
MCP23S17* mcp23s17;

Button buttons[] = {
    Button(
        BTN_PAUSE_MASK,
        []() {
            LOG_INFO(TAG, "toggle pause");
            Audio::togglePause();
        },
        BTN_POWEROFF_DELAY,
        []() {
            LOG_INFO(TAG, "prepare sleep");
            Power::prepareSleep();
        },
        []() {
            delay(DELAY_BEFORE_SLEEP);
            LOG_INFO(TAG, "sleep");
            Power::deepSleep();
        }),
    Button(BTN_VOLUME_DOWN_MASK, BTN_VOLUME_REPEAT,
           []() {
               LOG_INFO(TAG, "volume down");
               Audio::volumeDown();
           }),
    Button(BTN_VOLUME_UP_MASK, BTN_VOLUME_REPEAT,
           []() {
               LOG_INFO(TAG, "volume up");
               Audio::volumeUp();
           }),
    Button(
        BTN_PREVIOUS_MASK,
        []() {
            LOG_INFO(TAG, "previous");
            Audio::previous();
        },
        BTN_REWIND_DELAY,
        []() {
            Audio::rewind();
            LOG_INFO(TAG, "rewind");
        }),
    Button(BTN_NEXT_MASK, BTN_NEXT_REPEAT,
           []() {
               LOG_INFO(TAG, "next");
               Audio::next();
           }),
};

extern "C" IRAM_ATTR void gpioIsr() { xTaskNotifyFromISR(gpioTaskHandle, 0, eNoAction, NULL); }

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
        bool pendingInterrupt = xTaskNotifyWait(0x0, 0x0, &value, timeout) == pdTRUE;

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

    pinMode(PIN_MCP23S17_IRQ, INPUT);

    {
        Lock lock(spiMutex);

        mcp23s17->begin();
        mcp23s17->setMirror(false);
        mcp23s17->setInterruptOD(false);
        mcp23s17->setInterruptLevel(LOW);

        mcp23s17->pinMode(PIN_AMP_ENABLE_MCP, OUTPUT);
        mcp23s17->digitalWrite(PIN_AMP_ENABLE_MCP, LOW);

        mcp23s17->pinMode(PIN_LED_MCP, OUTPUT);
        mcp23s17->digitalWrite(PIN_LED_MCP, LOW);

        for (uint8_t i = 8; i < 13; i++) mcp23s17->pinMode(i, INPUT);
        for (uint8_t i = 8 + DIP_SWITCH_SHIFT; i <= 9 + DIP_SWITCH_SHIFT; i++) mcp23s17->pinMode(i, INPUT_PULLUP);
        for (uint8_t i = TP5600_STATUS_SHIFT; i <= 1 + TP5600_STATUS_SHIFT; i++) mcp23s17->pinMode(i, INPUT_PULLUP);
    }
}

void Gpio::start() {
    xTaskCreatePinnedToCore(gpioTask, "gpio", STACK_SIZE_GPIO, NULL, TASK_PRIORITY_GPIO, &gpioTaskHandle, SERVICE_CORE);
}

void Gpio::enableAmp() {
    Lock lock(spiMutex);
    mcp23s17->digitalWrite(PIN_AMP_ENABLE_MCP, HIGH);
}

void Gpio::disableAmp() {
    Lock lock(spiMutex);
    mcp23s17->digitalWrite(PIN_AMP_ENABLE_MCP, LOW);
}

uint8_t Gpio::readConfigSwitches() {
    Lock lock(spiMutex);
    return (~mcp23s17->readPort(1) >> DIP_SWITCH_SHIFT) & 0x03;
}

uint8_t Gpio::readTP5600Status() {
    Lock lock(spiMutex);
    return (~mcp23s17->readPort(0) >> TP5600_STATUS_SHIFT) & 0x03;
}

void Gpio::switchLed(bool enable) {
    Lock lock(spiMutex);
    mcp23s17->digitalWrite(PIN_LED_MCP, enable ? HIGH : LOW);
}
