#ifndef CONFIG_HXX
#define CONFIG_HXX

#include <string>

#include "Command.hxx"

class Config {
   public:
    virtual const Command::Command& commandForRfid(const std::string& uid) = 0;

    virtual bool isRfidMapped(const std::string& uid) = 0;

   protected:
    Config() = default;
    Config(const Config&) = default;
    Config(Config&&) = default;

    Config& operator=(const Config&) = default;
    Config& operator=(Config&&) = default;
};

#endif  // CONFIG_HXX
