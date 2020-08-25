#include "JsonConfig.hxx"

#include <Arduino.h>
#include <ArduinoJson.h>
#include <SD.h>
#include <StreamUtils.h>

namespace {

class SPIRamAllocator {
   public:
    void* allocate(size_t n) { return ps_malloc(n); }

    void deallocate(void* p) { free(p); }
};

}  // namespace

using SPIRamJsonDocument = BasicJsonDocument<SPIRamAllocator>;

bool JsonConfig::load() {
    File configFile = SD.open("/config.json");
    if (!configFile) {
        Serial.println("config.json not found");
        return false;
    }

    SPIRamJsonDocument configJson(configFile.size() * 2);
    ReadBufferingStream stream(configFile, 64);

    DeserializationError err = deserializeJson(configJson, stream);
    configFile.close();

    if (err != DeserializationError::Ok) {
        Serial.printf("Failed to parse config.json: %s\r\n", err.c_str());

        return false;
    }

    auto rfidMapping = configJson["rfidMapping"];

    rfidMap.clear();

    if (!rfidMapping.is<JsonObject>()) {
        Serial.println("WARNING: config contains no RFID mappings");
    } else {
        rfidMap.reserve(rfidMapping.as<JsonObject>().size());

        for (auto mapping : rfidMapping.as<JsonObject>()) {
            Serial.printf("RFID mapping: %s -> %s\r\n", mapping.key().c_str(), mapping.value().as<const char*>());

            rfidMap[mapping.key().c_str()] = mapping.value().as<std::string>();
        }
    }

    return true;
}

bool JsonConfig::isRfidConfigured(const std::string& uid) const { return rfidMap.find(uid) != rfidMap.end(); }

std::string JsonConfig::albumForRfid(const std::string& uid) const {
    return isRfidConfigured(uid) ? rfidMap.at(uid) : "";
}
