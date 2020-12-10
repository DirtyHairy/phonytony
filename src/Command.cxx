#include "Command.hxx"

#include <utility>

#include "Audio.hxx"
#include "Config.hxx"
#include "Power.hxx"
#include "net/Net.hxx"

namespace Command {

Command::~Command() {
    if (type == Type::play) payload.track.~basic_string();
}

Command::Command(const Command& cmd) {
    type = cmd.type;

    switch (type) {
        case Type::play:
            new (&(payload.track)) std::string(cmd.payload.track);
            break;

        case Type::dbgSetVoltage:
            this->payload.voltage = cmd.payload.voltage;
            break;

        default:
            break;
    }
}

Command::Command(Command&& cmd) {
    type = cmd.type;

    switch (type) {
        case Type::play:
            new (&(payload.track)) std::string(std::move(cmd.payload.track));
            break;

        case Type::dbgSetVoltage:
            this->payload.voltage = cmd.payload.voltage;
            break;

        default:
            break;
    }
}

Command Command::play(const char* track) {
    Command cmd;

    cmd.type = Type::play;
    new (&cmd.payload.track) std::string(track);

    return cmd;
}

Command Command::dbgSetVoltage(uint32_t voltage) {
    Command cmd;

    cmd.type = Type::dbgSetVoltage;
    cmd.payload.voltage = voltage;

    return cmd;
}

Command Command::startNet() {
    Command cmd;

    cmd.type = Type::startNet;

    return cmd;
}

Command Command::stopNet() {
    Command cmd;

    cmd.type = Type::stopNet;

    return cmd;
}

Command Command::none() { return Command(); }

void dispatch(const Command& cmd) {
    switch (cmd.type) {
        case Command::Type::play:
            Audio::play(cmd.payload.track.c_str());
            break;

        case Command::Type::dbgSetVoltage:
            Audio::signalCommandReceived();
            Power::dbgSetVoltage(cmd.payload.voltage);
            break;

        case Command::Type::startNet:
            Audio::signalCommandReceived();
            Net::start();
            break;

        case Command::Type::stopNet:
            Audio::signalCommandReceived();
            Net::stop();
            break;

        default:
            break;
    }
}

}  // namespace Command
