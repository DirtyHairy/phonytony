#ifndef COMMAND_HXX
#define COMMAND_HXX

#include <cstdint>
#include <string>

namespace Command {

struct Command {
    enum class Type : uint8_t { play, dbgSetVoltage, startNet, stopNet, none };

    union Payload {
        std::string track;
        uint32_t voltage;

        ~Payload() {}
        Payload() {}
    };

    Type type;
    Payload payload;

    static Command play(const char* track);
    static Command dbgSetVoltage(uint32_t voltage);
    static Command startNet();
    static Command stopNet();
    static Command none();

    ~Command();
    Command(const Command&);
    Command(Command&&);

   private:
    Command() = default;
    Command& operator=(const Command&) = delete;
    Command& operator=(Command&&) = delete;
};

void dispatch(const Command& cmd);

}  // namespace Command

#endif  // COMMAND_HXX
