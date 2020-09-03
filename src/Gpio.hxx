#ifndef GPIO_HXX
#define GPIO_HXX

#include <cstdint>

class SPIClass;

namespace Gpio {

void initialize(SPIClass& spi, void* spiMutex);

void start();

void enableAmp();

void disableAmp();

uint8_t readConfigSwitches();

uint8_t readTP4200Status();

void switchLed(bool enable);

void prepareSleep();

}  // namespace Gpio

#endif  // GPIO_HXX
