#include <Arduino.h>

// clang-format off
#include <freertos/FreeRTOS.h>

#include <mad.h>
// clang-format on

#include <SD.h>
#include <SPI.h>
#include <freertos/semphr.h>

#include "Audio.hxx"
#include "Gpio.hxx"
#include "JsonConfig.hxx"
#include "Led.hxx"
#include "Log.hxx"
#include "Power.hxx"
#include "Rfid.hxx"
#include "Watchdog.hxx"
#include "config.h"

static SPIClass spiVSPI(VSPI);
static SPIClass spiHSPI(HSPI);
static SemaphoreHandle_t hspiMutex;
static JsonConfig config;

#define TAG "main"

bool setupSd() {
    if (!SD.begin(PIN_SD_CS, spiVSPI, SPI_FREQ_SD)) {
        LOG_ERROR(TAG, "Failed to initialize SD");
        return false;
    }

    LOG_INFO(TAG, "SD card initialized OK");

    return true;
}

void printStats() {
    LOG_INFO(TAG, "ESP-IDF version: %s", esp_get_idf_version());
    LOG_INFO(TAG, "free heap: %u bytes", esp_get_free_heap_size());
}

void setup() {
    setCpuFrequencyMhz(CPU_FREQUENCY);

    Log::initialize();

    hspiMutex = xSemaphoreCreateMutex();
    spiVSPI.begin();
    spiHSPI.begin();
    spiHSPI.setFrequency(HSPI_FREQ);

    Gpio::initialize(spiHSPI, hspiMutex);

    LOG_INFO(TAG, "TP4200 status: %i", (int)Gpio::readTP4200Status());

    if (Power::isResumeFromSleep())
        LOG_INFO(TAG, "resuming from sleep...");
    else
        LOG_INFO(TAG, "starting from hard reset...");

    pinMode(PIN_POWER, OUTPUT);
    digitalWrite(PIN_POWER, 0);

    delay(POWER_ON_DELAY);

    LOG_INFO(TAG, "power enabled");

    if (!setupSd()) {
        return;
    }

    Log::enableSD();

    if (!config.load()) {
        LOG_WARN(TAG, "WARNING: failed to load configuration");
    }

    Audio::initialize();
    Rfid::initialize(spiHSPI, hspiMutex, config);
    Power::initialize();
    Watchdog::initialize();
    Led::initialize();

    Audio::start();
    Led::start();
    Rfid::start();
    Gpio::start();
    Watchdog::start();

    LOG_INFO(TAG, "DIP config %i", (int)Gpio::readConfigSwitches());
    delay(500);
    printStats();
}

void loop() { vTaskDelete(NULL); }
