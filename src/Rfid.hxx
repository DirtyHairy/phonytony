#ifndef RFID_HXX
#define RFID_HXX

class SPIClass;

namespace Rfid {

void initialize(SPIClass& spi, void* spiMutex);

void start();

}  // namespace Rfid

#endif  // RFID_HXX
