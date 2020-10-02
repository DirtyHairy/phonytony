#ifndef POWER_HXX
#define POWER_HXX

#include <cstdint>

namespace Power {

struct BatteryState {
    enum State : uint8_t { discharging = 0, charging = 1, standby = 2 };
    enum Level : uint8_t { poweroff = 0, critical = 1, low = 2, full = 3 };

    State state;
    uint32_t voltage;
    Level level{full};
};

void initialize();

void start();

void deepSleep();

bool isResumeFromSleep();

void dbgSetVoltage(uint32_t voltage);

BatteryState getBatteryState();

}  // namespace Power

#endif  // POWER_HXX
