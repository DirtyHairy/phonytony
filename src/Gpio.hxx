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

}  // namespace Gpio

#endif  // GPIO_HXX
