#ifndef JSON_CONFIG_HXX
#define JSON_CONFIG_HXX

#include <string>
#include <unordered_map>

#include "Command.hxx"
#include "Config.hxx"

class JsonConfig : public Config {
   public:
    JsonConfig() = default;

    bool load();

    const Command::Command& commandForRfid(const std::string& uid) override;

    bool isRfidMapped(const std::string& uid) override;

   private:
    std::unordered_map<std::string, Command::Command> rfidMap;
};

#endif  // JSON_CONFIG_HXX
