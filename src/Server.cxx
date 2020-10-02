#include "Server.hxx"

// clang-format off
#include <freertos/FreeRTOS.h>
// clang-format on

#include <ArduinoJson.h>
#include <ESPAsyncWebServer.h>
#include <SPIFFS.h>
#include <freertos/semphr.h>
#include <freertos/task.h>

#include <atomic>

#include "Audio.hxx"
#include "Lock.hxx"
#include "Power.hxx"
#include "config.h"

namespace {

class AsyncEventSourceWithCORS : public AsyncWebHandler {
   public:
    AsyncEventSourceWithCORS(AsyncEventSource& delegateSource) : delegateSource(delegateSource) {}

    virtual bool canHandle(AsyncWebServerRequest* request) override final { return delegateSource.canHandle(request); }

    virtual void handleRequest(AsyncWebServerRequest* request) override final {
        AsyncEventSourceResponse* response = new AsyncEventSourceResponse(&delegateSource);

        response->addHeader("Access-Control-Allow-Origin", "*");

        request->send(response);
    }

   private:
    AsyncEventSource& delegateSource;
};

TaskHandle_t serverTaskHandle = nullptr;
SemaphoreHandle_t messageMutex;
SemaphoreHandle_t startStopMutex;

AsyncWebServer server(80);
AsyncEventSource events("/events");
AsyncEventSourceWithCORS eventsHandler(events);
static char serializedMessage[1024] = "";

std::atomic<bool> isRunning;

void sendUpdate() {
    StaticJsonDocument<1024> json;
    JsonObject audio = json.createNestedObject("audio");
    JsonObject power = json.createNestedObject("power");
    Power::BatteryState batteryState = Power::getBatteryState();

    audio["isPlaying"] = Audio::isPlaying();
    audio["currentAlbum"] = Audio::currentAlbum();
    audio["currentTrack"] = Audio::currentTrack();
    audio["volume"] = Audio::currentVolume();

    power["voltage"] = batteryState.voltage;
    power["level"] = static_cast<uint8_t>(batteryState.level);
    power["state"] = static_cast<uint8_t>(batteryState.state);

    json["heap"] = esp_get_free_heap_size();

    Lock lock(messageMutex);

    serializeJson(json, serializedMessage, 1024);

    events.send(serializedMessage, "update", 0, 1000);
}

void _serverTask() {
    sendUpdate();

    while (true) {
        uint32_t value;

        xTaskNotifyWait(0x0, 0x0, &value, isRunning ? 1000 : portMAX_DELAY);

        sendUpdate();
    }
}

void serverTask(void*) {
    _serverTask();

    vTaskDelete(NULL);
}

void notFound(AsyncWebServerRequest* request) { request->send(404, "text/plain", "Not Found"); }

const String templateProcessor(const String& key) { return key == "FROM_BOX" ? "yes" : ""; }

void setupServer() {
    // server-side events
    events.onConnect([](AsyncEventSourceClient* client) {
        Lock lock(messageMutex);

        client->send(serializedMessage, "update", 0, 1000);
    });
    server.addHandler(&eventsHandler);

    // templating for index.html
    server.on("/index.html", HTTP_GET, [](AsyncWebServerRequest* request) {
        request->send(SPIFFS, "/index.html", "text/html", false, templateProcessor);
    });

    // favicon has 10 minutes cache lifetime
    server.on("/favicon.ico", HTTP_GET, [](AsyncWebServerRequest* request) {
        AsyncWebServerResponse* response = request->beginResponse(SPIFFS, "/favicon.ico");
        response->addHeader("Cache-Control", "max-age=600");

        request->send(response);
    });

    // all other resources have contend hashes in their URI and are cached indefinitely (aka 1 year)
    // serve straight from SPIFFS
    server.serveStatic("/", SPIFFS, "/").setCacheControl("max-age=31536000");

    // index redirect
    server.on("/", HTTP_ANY, [](AsyncWebServerRequest* request) { request->redirect("/index.html"); });

    // 404
    server.onNotFound(notFound);
}

}  // namespace

void HTTPServer::initialize() {
    isRunning = false;
    messageMutex = xSemaphoreCreateMutex();
    startStopMutex = xSemaphoreCreateMutex();

    SPIFFS.begin();

    setupServer();
}

void HTTPServer::start() {
    if (isRunning) return;

    Lock lock(startStopMutex);

    server.begin();

    if (serverTaskHandle)
        xTaskNotify(serverTaskHandle, 0, eNoAction);
    else
        xTaskCreatePinnedToCore(serverTask, "server", STACK_SIZE_SERVER, NULL, TASK_PRIORITY_SERVER, &serverTaskHandle,
                                SERVICE_CORE);

    isRunning = true;
}

void HTTPServer::stop() {
    if (!isRunning) return;

    Lock lock(startStopMutex);

    server.end();
    isRunning = false;
}

void HTTPServer::sendUpdate() {
    if (isRunning) xTaskNotify(serverTaskHandle, 0, eNoAction);
}
