#ifndef GPIO_TASK_HXX
#define GPIO_TASK_HXX

class SPIClass;

namespace GpioTask {

void initialize(SPIClass& spi, void* spiMutex);

void start();

}  // namespace GpioTask

#endif  // GPIO_TASK_HXX
