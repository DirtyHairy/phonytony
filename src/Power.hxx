#ifndef POWER_HXX
#define POWER_HXX

namespace Power {

void initialize();

void prepareSleep();

void deepSleep();

bool isResumeFromSleep();

}  // namespace Power

#endif  // POWER_HXX
