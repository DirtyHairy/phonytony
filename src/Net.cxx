#include "Net.hxx"

// clang-format off
#include <freertos/FreeRTOS.h>
// clang-format on

#include <ESPmDNS.h>
#include <WiFi.h>
#include <freertos/semphr.h>
#include <freertos/task.h>

#include <atomic>

#include "Lock.hxx"
#include "Log.hxx"
#include "config.h"
#include "local_config.h"

#define TAG "net"

namespace {
DRAM_ATTR TaskHandle_t networkTaskHandle = nullptr;
SemaphoreHandle_t startStopMutex;

std::atomic<bool> isShutdown;
bool initialized = false;

void init() {
    if (initialized) {
        WiFi.persistent(false);
        WiFi.mode(WIFI_STA);

        initialized = true;
    } else {
        esp_wifi_start();
        esp_wifi_set_mode(WIFI_MODE_STA);
    }

    WiFi.setHostname(HOSTNAME);
    WiFi.begin(SSID, PSK);

    LOG_INFO(TAG, "connecting to %s", SSID);

    while (!WiFi.isConnected()) {
        LOG_INFO(TAG, "connecting...");

        delay(1000);
    }

    LOG_INFO(TAG, "connected with IP %s", WiFi.localIP().toString().c_str());

    if (!MDNS.begin(HOSTNAME))
        LOG_ERROR(TAG, "failed to start mDNS responder");
    else
        LOG_INFO(TAG, "mDNS responder running");
}

void shutdown() {
    MDNS.end();

    esp_wifi_disconnect();
    esp_wifi_stop();
}

void _networkTask() {
    init();

    while (true) {
        delay(1000);
    }
}

void networkTask(void*) {
    _networkTask();

    vTaskDelete(NULL);

    shutdown();
}

}  // namespace

void Net::initialize() {
    isShutdown = false;

    startStopMutex = xSemaphoreCreateMutex();
}

void Net::start() {
    if (isShutdown) return;

    Lock lock(startStopMutex);

    LOG_INFO(TAG, "starting...");

    xTaskCreatePinnedToCore(networkTask, "net", STACK_SIZE_NET, NULL, TASK_PRIORITY_NET, &networkTaskHandle,
                            SERVICE_CORE);
}

void Net::stop() {
    Lock lock(startStopMutex);

    LOG_INFO(TAG, "stopping...");

    if (networkTaskHandle) {
        vTaskDelete(networkTaskHandle);

        networkTaskHandle = nullptr;

        shutdown();
    }
}

void Net::prepareSleep() {
    if (isShutdown) return;

    isShutdown = true;

    stop();
}
