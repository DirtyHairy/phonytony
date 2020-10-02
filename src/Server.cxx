#include "Server.hxx"

#include <ESPAsyncWebServer.h>
#include <SPIFFS.h>

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

AsyncWebServer server(80);
AsyncEventSource events("/events");
AsyncEventSourceWithCORS eventsHandler(events);
static char serializedMessage[1024] = "";

void notFound(AsyncWebServerRequest* request) { request->send(404, "text/plain", "Not Found"); }

const String templateProcessor(const String& key) { return key == "FROM_BOX" ? "yes" : ""; }

void setupServer() {
    // server-side events
    events.onConnect([](AsyncEventSourceClient* client) { client->send(serializedMessage, "update", 0, 1000); });
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
    SPIFFS.begin();

    setupServer();
    strcpy(serializedMessage, "hulpe");
}

void HTTPServer::start() { server.begin(); }

void HTTPServer::stop() { server.end(); }
