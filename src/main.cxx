#include <Arduino.h>

// clang-format off
#include <freertos/FreeRTOS.h>

#include <mad.h>
// clang-format on

#include <esp_vfs_fat.h>
#include <driver/sdspi_host.h>
#include <SPI.h>
#include <freertos/semphr.h>

#include "Audio.hxx"
#include "Gpio.hxx"
#include "JsonConfig.hxx"
#include "Led.hxx"
#include "Log.hxx"
#include "Net.hxx"
#include "Power.hxx"
#include "Rfid.hxx"
#include "Server.hxx"
#include "Watchdog.hxx"
#include "config.h"

static SPIClass spiVSPI(VSPI);
static SPIClass spiHSPI(HSPI);
static SemaphoreHandle_t hspiMutex;
static JsonConfig config;

#define TAG "main"

bool setupSd() {
    sdmmc_host_t host = SDSPI_HOST_DEFAULT();
    sdspi_slot_config_t slot_config = SDSPI_SLOT_CONFIG_DEFAULT();

    host.slot = VSPI_HOST;
    host.max_freq_khz = 80000;

    slot_config.gpio_cs = PIN_SD_CS;
    slot_config.gpio_sck = GPIO_NUM_18;
    slot_config.gpio_miso = GPIO_NUM_19;
    slot_config.gpio_mosi = GPIO_NUM_23;

    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
        .format_if_mount_failed = false, .max_files = 5, .allocation_unit_size = 16 * 1024};

    sdmmc_card_t* card;
    esp_err_t ret = esp_vfs_fat_sdmmc_mount("/sdcard", &host, &slot_config, &mount_config, &card);

    if (ret != ESP_OK) {
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

void logBatteryState() {
    const char* state;
    const char* level;
    Power::BatteryState batteryState = Power::getBatteryState();

    switch (batteryState.state) {
        case Power::BatteryState::charging:
            state = "charging";
            break;

        case Power::BatteryState::discharging:
            state = "discharging";
            break;

        case Power::BatteryState::standby:
        default:
            state = "standby";
            break;
    }

    switch (batteryState.level) {
        case Power::BatteryState::Level::full:
            level = "full";
            break;

        case Power::BatteryState::Level::low:
            level = "low";
            break;

        case Power::BatteryState::Level::critical:
            level = "critical";
            break;

        case Power::BatteryState::Level::poweroff:
        default:
            level = "not sufficient";
            break;
    }

    LOG_INFO(TAG, "battery %s and %s at %i mV", level, state, batteryState.voltage);
}

void setup() {
    setCpuFrequencyMhz(240);

    Log::initialize();

    pinMode(PIN_MCP23S17_CS, OUTPUT);
    digitalWrite(PIN_MCP23S17_CS, HIGH);

    pinMode(PIN_RFID_CS, OUTPUT);
    digitalWrite(PIN_RFID_CS, HIGH);

    hspiMutex = xSemaphoreCreateMutex();
    spiVSPI.begin();
    spiHSPI.begin();
    spiHSPI.setFrequency(HSPI_FREQ);

    Gpio::initialize(spiHSPI, hspiMutex);
    bool silentStart = Gpio::silentStart();

    Power::initialize();

    logBatteryState();

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

    Audio::initialize();
    Rfid::initialize(spiHSPI, hspiMutex, config);
    Watchdog::initialize();
    Led::initialize();
    Net::initialize();
    HTTPServer::initialize();

    if (!config.load()) {
        LOG_WARN(TAG, "WARNING: failed to load configuration");
    }

    Audio::start(silentStart);
    Gpio::start();
    Led::start();
    Power::start();
    Watchdog::start();

    Rfid::start();

    Log::enableSD();

    setCpuFrequencyMhz(CPU_FREQUENCY);

    LOG_INFO(TAG, "DIP config %i", (int)Gpio::readConfigSwitches());
    delay(500);
    printStats();
}

void loop() { vTaskDelete(NULL); }
