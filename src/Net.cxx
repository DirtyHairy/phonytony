#include "Net.hxx"

// clang-format off
#include <freertos/FreeRTOS.h>
// clang-format on

#include <Arduino.h>
#include <esp_event_loop.h>
#include <esp_wifi.h>
#include <freertos/semphr.h>
#include <freertos/task.h>
#include <lwip/apps/sntp.h>
#include <mdns.h>

#include <atomic>

#include "Lock.hxx"
#include "Log.hxx"
#include "Server.hxx"
#include "config.h"
#include "local_config.h"

#define TAG "net"

namespace {

std::atomic<bool> networkInitialized;
std::atomic<bool> isRunning;
bool firstConnectComplete;

void startMdns() {
    if (mdns_init()) {
        LOG_ERROR(TAG, "failed to start MDNS");
        return;
    }

    mdns_hostname_set(HOSTNAME);
    mdns_instance_name_set(HOSTNAME);

    LOG_INFO(TAG, "MDNS up and running");
}

void startSntp() {
    sntp_setoperatingmode(SNTP_OPMODE_POLL);
    sntp_setservername(0, (char *)"pool.ntp.org");
    sntp_init();

    while (sntp_get_sync_status() != SNTP_SYNC_STATUS_COMPLETED) delay(1);

    LOG_INFO(TAG, "SNTP sync complete");
}

esp_err_t eventHandler(void *, system_event_t *event) {
    switch (event->event_id) {
        case SYSTEM_EVENT_STA_START:
            LOG_INFO(TAG, "STA started");

            if (tcpip_adapter_set_hostname(TCPIP_ADAPTER_IF_STA, HOSTNAME) != ESP_OK)
                LOG_ERROR(TAG, "failed to configure hostname");

            if (esp_wifi_connect() != ESP_OK) LOG_ERROR(TAG, "wifi connect failed");

            break;

        case SYSTEM_EVENT_STA_STOP:
            LOG_INFO(TAG, "STA stopped");

            mdns_free();
            sntp_stop();
            HTTPServer::stop();

            break;

        case SYSTEM_EVENT_STA_CONNECTED: {
            uint8_t *bssid = event->event_info.connected.bssid;
            LOG_INFO(TAG, "connected to %s / %02x:%02x:%02x:%02x:%02x:%02x", event->event_info.connected.ssid, bssid[0],
                     bssid[1], bssid[2], bssid[3], bssid[4], bssid[5]);
        }

        break;

        case SYSTEM_EVENT_STA_GOT_IP:
            LOG_INFO(TAG, "got IP %s", ip4addr_ntoa(&event->event_info.got_ip.ip_info.ip));

            startMdns();

            if (!firstConnectComplete) {
                startSntp();
            }

            HTTPServer::start();

            firstConnectComplete = true;

            break;

        case SYSTEM_EVENT_STA_DISCONNECTED:
            LOG_INFO(TAG, "wifi disconnected");

            break;

        default:
            break;
    }

    return ESP_OK;
}

}  // namespace

void Net::initialize() {
    networkInitialized = false;
    isRunning = false;
}

void Net::start() {
    if (isRunning) return;

    if (!networkInitialized) {
        tcpip_adapter_init();
        esp_event_loop_init(eventHandler, nullptr);

        wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
        if (esp_wifi_init(&cfg) != ESP_OK || esp_wifi_set_storage(WIFI_STORAGE_RAM)) {
            Serial.println("failed to init wifi");
            return;
        }

        static wifi_config_t wifiConfig;
        strcpy((char *)&wifiConfig.sta.ssid, SSID);
        strcpy((char *)&wifiConfig.sta.password, PSK);
        wifiConfig.sta.sort_method = WIFI_CONNECT_AP_BY_SIGNAL;
        wifiConfig.sta.scan_method = WIFI_ALL_CHANNEL_SCAN;

        if (esp_wifi_set_mode(WIFI_MODE_STA) != ESP_OK || esp_wifi_set_config(ESP_IF_WIFI_STA, &wifiConfig) != ESP_OK) {
            LOG_ERROR(TAG, "failed to configure wifi");
            return;
        }

        networkInitialized = true;
    }

    if (esp_wifi_start() != ESP_OK) {
        LOG_ERROR(TAG, "failed to start wifi");
        return;
    }

    isRunning = true;
}

void Net::stop() {
    if (!isRunning) return;

    HTTPServer::closeConnections();
    esp_wifi_stop();

    isRunning = false;
}

void Net::prepareSleep() { esp_wifi_stop(); }
