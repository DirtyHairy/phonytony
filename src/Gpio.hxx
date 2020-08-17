#ifndef GPIO_HXX
#define GPIO_HXX

class SPIClass;

namespace Gpio {

void initialize(SPIClass& spi, void* spiMutex);

void start();

void enableAmp();

void disableAmp();

}  // namespace Gpio

#endif  // GPIO_HXX
