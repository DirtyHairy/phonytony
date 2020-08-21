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
#include "Led.hxx"
#include "Power.hxx"
#include "Rfid.hxx"
#include "Watchdog.hxx"
#include "config.h"

static SPIClass spiVSPI(VSPI);
static SPIClass spiHSPI(HSPI);
static SemaphoreHandle_t hspiMutex;

bool probeSd() {
    if (!SD.begin(PIN_SD_CS, spiVSPI, SPI_FREQ_SD)) {
        Serial.println("Failed to initialize SD");
        return false;
    }

    Serial.println("SD card initialized OK");

    return true;
}

void printStats() {
    Serial.printf("ESP-IDF version: %s\r\n", esp_get_idf_version());
    Serial.printf("free heap: %u bytes\r\n", esp_get_free_heap_size());
}

void setup() {
    setCpuFrequencyMhz(CPU_FREQUENCY);
    hspiMutex = xSemaphoreCreateMutex();

    Serial.begin(115200);

    if (Power::isResumeFromSleep())
        Serial.println("resuming from sleep...");
    else
        Serial.println("starting from hard reset...");

    pinMode(PIN_POWER, OUTPUT);
    digitalWrite(PIN_POWER, 0);

    delay(POWER_ON_DELAY);

    Serial.println("power enabled");

    spiVSPI.begin();
    spiHSPI.begin();
    spiHSPI.setFrequency(HSPI_FREQ);

    if (!probeSd()) {
        return;
    }

    Audio::initialize();
    Rfid::initialize(spiHSPI, hspiMutex);
    Gpio::initialize(spiHSPI, hspiMutex);
    Power::initialize();
    Watchdog::initialize();
    Led::initialize();

    Led::start();
    Audio::start();
    Rfid::start();
    Gpio::start();
    Watchdog::start();

    Serial.printf("DIP config %i\r\n", Gpio::readConfigSwitches());
    delay(500);
    printStats();
}

void loop() { vTaskDelete(NULL); }
