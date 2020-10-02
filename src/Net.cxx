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
#include "Server.hxx"
#include "config.h"
#include "local_config.h"

#define TAG "net"

namespace {
SemaphoreHandle_t startStopMutex;

bool initialized = false;
std::atomic<bool> isShutdown;
std::atomic<bool> isRunning;

void startNet() {
    if (initialized) {
        esp_wifi_start();
        esp_wifi_set_mode(WIFI_MODE_STA);

        WiFi.setHostname(HOSTNAME);
        WiFi.begin(SSID, PSK);

    } else {
        HTTPServer::initialize();

        WiFi.persistent(false);
        WiFi.mode(WIFI_STA);

        WiFi.onEvent(
            [](system_event_t *event) { LOG_INFO(TAG, "wifi connected to %s", event->event_info.connected.ssid); },
            SYSTEM_EVENT_STA_CONNECTED);

        WiFi.onEvent([](system_event_id_t event) { LOG_INFO(TAG, "wifi disconnected"); },
                     SYSTEM_EVENT_STA_DISCONNECTED);

        WiFi.onEvent(
            [](system_event_id_t event) {
                LOG_INFO(TAG, "got IP %s", WiFi.localIP().toString().c_str());

                if (!MDNS.begin(HOSTNAME))
                    LOG_ERROR(TAG, "failed to start mDNS responder");
                else
                    LOG_INFO(TAG, "mdns responder running");

                HTTPServer::start();
            },
            SYSTEM_EVENT_STA_GOT_IP);

        WiFi.setHostname(HOSTNAME);
        WiFi.begin(SSID, PSK);

        initialized = true;
    }

    LOG_INFO(TAG, "wifi started, connecting to %s", SSID);
}

void stopNet() {
    if (!initialized) return;

    MDNS.end();
    HTTPServer::stop();

    WiFi.disconnect();
    esp_wifi_stop();

    LOG_INFO(TAG, "wifi stopped");
}

}  // namespace

void Net::initialize() {
    isShutdown = false;
    isRunning = false;
    startStopMutex = xSemaphoreCreateMutex();
}

void Net::start() {
    Lock lock(startStopMutex);

    if (isShutdown || isRunning) return;

    startNet();

    isRunning = true;
}

void Net::stop() {
    Lock lock(startStopMutex);

    if (isShutdown || !isRunning) return;

    stopNet();

    isRunning = false;
}

void Net::prepareSleep() {
    if (isShutdown) return;

    stop();

    isShutdown = true;
}
