#ifndef POWER_HXX
#define POWER_HXX

#include <cstdint>

namespace Power {

struct BatteryState {
    enum State { discharging, charging, standby };

    State state;
    uint32_t voltage;
};

void initialize();

void prepareSleep();

void deepSleep();

bool isResumeFromSleep();

BatteryState getBatteryState();

}  // namespace Power

#endif  // POWER_HXX
