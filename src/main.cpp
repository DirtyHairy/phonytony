#include <Arduino.h>

// clang-format off
#include <freertos/FreeRTOS.h>

#include <mad.h>
// clang-format on

#include "config.h"

#include <SD.h>
#include <SPI.h>
#include <freertos/semphr.h>

#include "Audio.hxx"
#include "Rfid.hxx"
#include "Gpio.hxx"

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

void setup() {
    setCpuFrequencyMhz(CPU_FREQUENCY);
    hspiMutex = xSemaphoreCreateMutex();

    Serial.begin(115200);

    pinMode(POWER_PIN, OUTPUT);
    digitalWrite(POWER_PIN, 0);

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

    Audio::start();
    Rfid::start();
    Gpio::start();
}

void loop() { vTaskDelete(NULL); }
