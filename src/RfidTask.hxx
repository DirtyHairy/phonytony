#ifndef RFID_TASK_HXX
#define RFID_TASK_HXX

class SPIClass;

namespace RfidTask {

void initialize(SPIClass& spi, void* spiMutex);

void start();

}  // namespace RfidTask

#endif  // RFID_TASK_HXX
