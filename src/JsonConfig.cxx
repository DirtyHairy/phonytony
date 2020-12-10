#include "JsonConfig.hxx"

#include <Arduino.h>
#include <ArduinoJson.h>
#include <StreamUtils.h>

#include <cstdio>
#include <memory>

#include "Log.hxx"
#include "PosixFile.h"

#define TAG "json"

using std::string;

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
    File configFile = PosixFile::open("/sdcard/config.json", "r");
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

        for (auto mapping : rfidMapping.as<JsonObject>()) {
            if (!processCommandDefinition(mapping.key().c_str(), mapping.value())) {
                LOG_WARN(TAG, "invalid mapping definition for UID %s", mapping.key().c_str());
            }
        }
    }

    return true;
}

bool JsonConfig::processCommandDefinition(const char* uid, const JsonVariant& definition) {
    if (definition.is<const char*>()) {
        rfidMap.emplace(uid, Command::Command::play(definition.as<const char*>()));
        return true;
    }

    if (!(definition.is<JsonObject>() && definition["type"].is<const char*>())) return false;

    string typeKey(definition["type"].as<const char*>());

    if (typeKey == "play") {
        if (!definition["album"].is<const char*>()) return false;

        rfidMap.emplace(uid, Command::Command::play(definition["album"].as<const char*>()));
        return true;

    } else if (typeKey == "dbgSetVoltage") {
        if (!definition["voltage"].is<uint32_t>()) return false;

        rfidMap.emplace(uid, Command::Command::dbgSetVoltage(definition["voltage"].as<uint32_t>()));
        return true;

    } else if (typeKey == "startNet") {
        rfidMap.emplace(uid, Command::Command::startNet());
        return true;

    } else if (typeKey == "stopNet") {
        rfidMap.emplace(uid, Command::Command::stopNet());
        return true;
    }

    return false;
}

bool JsonConfig::isRfidMapped(const string& uid) { return rfidMap.find(uid) != rfidMap.end(); }

const Command::Command& JsonConfig::commandForRfid(const std::string& uid) {
    return isRfidMapped(uid) ? rfidMap.at(uid) : emptyCommand;
}
