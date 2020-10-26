#include "Server.hxx"

// clang-format off
#include <freertos/FreeRTOS.h>
// clang-format on

#include <ArduinoJson.h>
#include <dirent.h>
#include <esp_http_server.h>
#include <esp_spiffs.h>
#include <freertos/semphr.h>
#include <freertos/task.h>
#include <sys/socket.h>
#include <sys/stat.h>

#include <atomic>
#include <string>

#include "Audio.hxx"
#include "Lock.hxx"
#include "Log.hxx"
#include "Power.hxx"
#include "config.h"

using std::string;

#define TAG "http"

namespace {

struct StaticFile {
    const char *mimeType;
    const char *cacheControl;
    bool compressed;
    size_t len;

    void *data;
};

constexpr int MAX_FILES = 10;

const char *MIME_TYPE_CSS = "text/css";
const char *MIME_TYPE_JAVASCRIPT = "text/javascript";
const char *MIME_TYPE_HTML = "text/html";
const char *MIME_TYPE_ICO = "image/x-icon";

const char *STATUS_SERVICE_UNAVAILABLE = "503 Service Unavailable";
const char *STATUS_OK = "200 OK";
const char *STATUS_MOVED_PERMANENTLY = "301 Moved Permanently";

const char *EVENT_TEMPLATE = "event: %s\r\ndata: %s\r\nretry: 1000\r\n\r\n";
size_t EVENT_BASE_LEN = strlen(EVENT_TEMPLATE) - 4;

constexpr int NUM_EVENT_SOCKETS = 5;
int eventSockets[NUM_EVENT_SOCKETS];

httpd_handle_t httpd_handle = nullptr;

TaskHandle_t serverTaskHandle = nullptr;
SemaphoreHandle_t statusMessageMutex;

char serializedStatusMessage[1024] = "";

std::atomic<bool> isRunning;

void freeCtx_events(void *ctx) {
    LOG_INFO(TAG, "event stream: client disconnected");

    *(int *)ctx = -1;
}

bool socketSendAll(int socket, const void *buffer, size_t len) {
    ssize_t sent;

    while ((sent = send(socket, buffer, len, 0)) < len) {
        if (sent < 0) return false;

        len -= sent;
        buffer = (void *)((char *)buffer + sent);
    }

    return true;
}

bool socketSendStr(int socket, const char *text) { return socketSendAll(socket, (void *)text, strlen(text)); }

bool sendEvent(int socket, const char *event, const char *data) {
    size_t eventLen = strlen(event);
    size_t dataLen = strlen(data);
    size_t payloadLen = EVENT_BASE_LEN + eventLen + dataLen;

    char payload[payloadLen + 1];
    snprintf(payload, payloadLen + 1, EVENT_TEMPLATE, event, data);

    bool res = socketSendAll(socket, (void *)payload, payloadLen);
    httpd_sess_update_lru_counter(httpd_handle, socket);

    return res;
}

bool sendEvent(const char *event, const char *data) {
    bool res = true;

    for (int i = 0; i < NUM_EVENT_SOCKETS; i++) {
        if (eventSockets[i] != -1) res &= sendEvent(eventSockets[i], event, data);
    }

    return res;
}

string getSuffix(string name) {
    size_t dot = name.find_last_of(".");
    if (dot == string::npos) return "";

    return name.substr(dot, string::npos);
}

string stripSuffix(string name) {
    size_t dot = name.find_last_of(".");
    if (dot == string::npos) return "";

    return name.substr(0, dot);
}

const char *getMimeType(string name) {
    string suffix = getSuffix(name);

    if (suffix == ".html") return MIME_TYPE_HTML;
    if (suffix == ".js") return MIME_TYPE_JAVASCRIPT;
    if (suffix == ".css") return MIME_TYPE_CSS;
    if (suffix == ".ico") return MIME_TYPE_ICO;

    return nullptr;
}

const char *getCacheControl(string name) {
    if (name == "index.html") return nullptr;
    if (name == "favicon.ico") return "max-age=600";

    return "max-age=525600";
}

esp_err_t requestHandler_events(httpd_req_t *req) {
    int iconn;
    for (iconn = 0; iconn < NUM_EVENT_SOCKETS; iconn++) {
        if (eventSockets[iconn] == -1) break;
    }

    if (iconn == NUM_EVENT_SOCKETS) {
        httpd_resp_set_status(req, STATUS_SERVICE_UNAVAILABLE);
        httpd_resp_send(req, nullptr, 0);

        return ESP_OK;
    }

    int socket = eventSockets[iconn] = httpd_req_to_sockfd(req);

    httpd_sess_set_ctx(httpd_handle, socket, &eventSockets[iconn], freeCtx_events);

    socketSendStr(socket,
                  "HTTP/1.1 200 OK\r\nContent-Type: text/event-stream\r\nAccess-Control-Allow-Origin: *\r\n\r\n");

    {
        Lock lock(statusMessageMutex);
        sendEvent(socket, "status", serializedStatusMessage);
    }

    LOG_INFO(TAG, "event stream: client connected");

    return ESP_OK;
}

esp_err_t requestHandler_file(httpd_req_t *req) {
    StaticFile *staticFile = (StaticFile *)req->user_ctx;

    httpd_resp_set_type(req, staticFile->mimeType);
    httpd_resp_set_hdr(req, "Content-Encoding", staticFile->compressed ? "gzip" : "identity");
    if (staticFile->cacheControl) httpd_resp_set_hdr(req, "Cache-Control", staticFile->cacheControl);
    httpd_resp_set_status(req, STATUS_OK);

    httpd_resp_send(req, (const char *)staticFile->data, staticFile->len);

    return ESP_OK;
}

esp_err_t requestHandler_redirect(httpd_req_t *req) {
    httpd_resp_set_status(req, STATUS_MOVED_PERMANENTLY);
    httpd_resp_set_hdr(req, "Location", (const char *)req->user_ctx);

    httpd_resp_send(req, nullptr, 0);

    return ESP_OK;
}

void registerStaticFile(string name) {
    bool isGz = getSuffix(name) == ".gz";
    string normalizedName = isGz ? stripSuffix(name) : name;
    string url = "/" + normalizedName;
    const char *mimeType = getMimeType(normalizedName);

    if (mimeType == nullptr) {
        LOG_WARN(TAG, "warning: ignoring file %s with unknown mime type", name.c_str());
        return;
    }

    string path = "/spiffs/" + name;
    struct stat fs;

    if (stat(path.c_str(), &fs) != 0) {
        LOG_ERROR(TAG, "unable to stat %s", path.c_str());
        return;
    }

    FILE *file = fopen(path.c_str(), "r");
    if (!file) {
        LOG_ERROR(TAG, "unable to open %s", path.c_str());
        return;
    }

    uint8_t *buffer = (uint8_t *)ps_malloc(fs.st_size);

    size_t bytesRead = 0;
    while (bytesRead < fs.st_size) {
        size_t r = fread((void *)(buffer + bytesRead), 1, fs.st_size - bytesRead, file);

        if (r == 0) {
            LOG_ERROR(TAG, "error reading %s", path.c_str());
            break;
        }

        bytesRead += r;
    }

    fclose(file);

    StaticFile *staticFile = new StaticFile();
    staticFile->compressed = isGz;
    staticFile->len = fs.st_size;
    staticFile->mimeType = mimeType;
    staticFile->data = (void *)buffer;
    staticFile->cacheControl = getCacheControl(normalizedName);

    httpd_uri_t uri = {
        .uri = url.c_str(), .method = HTTP_GET, .handler = requestHandler_file, .user_ctx = (void *)staticFile};
    httpd_register_uri_handler(httpd_handle, &uri);
}

void registerStaticFiles() {
    DIR *dir = opendir("/spiffs");

    if (!dir) {
        LOG_ERROR(TAG, "failed to open /spiffs");
        return;
    }

    struct dirent *entry;
    while ((entry = readdir(dir))) registerStaticFile(entry->d_name);

    closedir(dir);
}

void updateStatusMessage() {
    StaticJsonDocument<1024> json;
    JsonObject audio = json.createNestedObject("audio");
    JsonObject power = json.createNestedObject("power");
    JsonObject heap = json.createNestedObject("heap");
    Power::BatteryState batteryState = Power::getBatteryState();

    audio["isPlaying"] = Audio::isPlaying();
    audio["currentAlbum"] = Audio::currentAlbum();
    audio["currentTrack"] = Audio::currentTrack();
    audio["volume"] = Audio::currentVolume();

    power["voltage"] = batteryState.voltage;
    power["level"] = static_cast<uint8_t>(batteryState.level);
    power["state"] = static_cast<uint8_t>(batteryState.state);

    heap["freeDRAM"] = heap_caps_get_free_size(MALLOC_CAP_DEFAULT | MALLOC_CAP_INTERNAL);
    heap["freePSRAM"] = heap_caps_get_free_size(MALLOC_CAP_DEFAULT | MALLOC_CAP_SPIRAM);
    heap["largestBlockDRAM"] = heap_caps_get_largest_free_block(MALLOC_CAP_DEFAULT | MALLOC_CAP_INTERNAL);
    heap["largestBlockPSRAM"] = heap_caps_get_largest_free_block(MALLOC_CAP_DEFAULT | MALLOC_CAP_SPIRAM);

    Lock lock(statusMessageMutex);
    serializeJson(json, serializedStatusMessage, 1024);
}

void initSpiffs() {
    esp_vfs_spiffs_conf_t cfg = {
        .base_path = "/spiffs", .partition_label = nullptr, .max_files = MAX_FILES, .format_if_mount_failed = true};

    if (esp_vfs_spiffs_register(&cfg) != ESP_OK)
        LOG_ERROR(TAG, "failed to setup SPIFFS");
    else
        LOG_INFO(TAG, "SPIFFS initialized");
}

void startServer() {
    if (httpd_handle) return;

    initSpiffs();
    updateStatusMessage();

    for (int i = 0; i < NUM_EVENT_SOCKETS; i++) eventSockets[i] = -1;

    httpd_config_t cfg = HTTPD_DEFAULT_CONFIG();
    cfg.lru_purge_enable = true;
    cfg.max_open_sockets = 12;
    cfg.max_uri_handlers = 20;

    if (httpd_start(&httpd_handle, &cfg) != ESP_OK) {
        LOG_ERROR(TAG, "failed to start HTTP server");
        return;
    }

    httpd_uri_t uri_events = {
        .uri = "/api/events", .method = HTTP_GET, .handler = requestHandler_events, .user_ctx = nullptr};
    httpd_register_uri_handler(httpd_handle, &uri_events);

    httpd_uri_t uri_root = {
        .uri = "/", .method = HTTP_GET, .handler = requestHandler_redirect, .user_ctx = (void *)"/index.html"};
    httpd_register_uri_handler(httpd_handle, &uri_root);

    registerStaticFiles();

    LOG_INFO(TAG, "server intialized");
}

void _serverTask() {
    while (true) {
        uint32_t value;

        xTaskNotifyWait(0x0, 0x0, &value, isRunning ? 1000 : portMAX_DELAY);

        updateStatusMessage();

        Lock lock(statusMessageMutex);
        sendEvent("status", serializedStatusMessage);
    }
}

void serverTask(void *) {
    _serverTask();

    vTaskDelete(NULL);
}

}  // namespace

void HTTPServer::initialize() {
    isRunning = false;
    statusMessageMutex = xSemaphoreCreateMutex();
}

void HTTPServer::start() {
    if (isRunning) return;

    startServer();

    if (serverTaskHandle)
        xTaskNotify(serverTaskHandle, 0, eNoAction);
    else
        xTaskCreatePinnedToCore(serverTask, "server", STACK_SIZE_SERVER, NULL, TASK_PRIORITY_SERVER, &serverTaskHandle,
                                SERVICE_CORE);

    isRunning = true;

    LOG_INFO(TAG, "server running");
}

void HTTPServer::stop() {
    if (!isRunning) return;

    isRunning = false;

    sendUpdate();

    LOG_INFO(TAG, "server stopped");
}

void HTTPServer::sendUpdate() {
    if (isRunning) xTaskNotify(serverTaskHandle, 0, eNoAction);
}
