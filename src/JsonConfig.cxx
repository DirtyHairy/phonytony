#include "JsonConfig.hxx"

#include <Arduino.h>
#include <ArduinoJson.h>
#include <SD.h>
#include <StreamUtils.h>

#include "Log.hxx"

#define TAG "json"

namespace {

Command::Command emptyCommand(Command::Command::none());

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
        LOG_WARN(TAG, "config.json not found");
        return false;
    }

    SPIRamJsonDocument configJson(configFile.size() * 2);
    ReadBufferingStream stream(configFile, 64);

    DeserializationError err = deserializeJson(configJson, stream);
    configFile.close();

    if (err != DeserializationError::Ok) {
        LOG_ERROR(TAG, "Failed to parse config.json: %s", err.c_str());

        return false;
    }

    auto rfidMapping = configJson["rfidMapping"];

    rfidMap.clear();

    if (!rfidMapping.is<JsonObject>()) {
        LOG_WARN(TAG, "config contains no RFID mappings");
    } else {
        rfidMap.reserve(rfidMapping.as<JsonObject>().size());

        for (auto mapping : rfidMapping.as<JsonObject>())
            rfidMap.emplace(mapping.key().c_str(), Command::Command::play(mapping.value().as<const char*>()));
    }

    return true;
}

bool JsonConfig::isRfidMapped(const std::string& uid) { return rfidMap.find(uid) != rfidMap.end(); }

const Command::Command& JsonConfig::commandForRfid(const std::string& uid) {
    return isRfidMapped(uid) ? rfidMap.at(uid) : emptyCommand;
}
