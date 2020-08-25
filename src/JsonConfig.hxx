#ifndef JSON_CONFIG_HXX
#define JSON_CONFIG_HXX

#include <string>
#include <unordered_map>

class JsonConfig {
   public:
    JsonConfig() = default;

    bool load();

    std::string albumForRfid(const std::string& rfid) const;

    bool isRfidConfigured(const std::string& uid) const;

   private:
    std::unordered_map<std::string, std::string> rfidMap;
};

#endif  // JSON_CONFIG_HXX
