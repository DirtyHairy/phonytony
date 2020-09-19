#ifndef RFID_HXX
#define RFID_HXX

class Config;
class SPIClass;

namespace Rfid {

void initialize(SPIClass& spi, void* spiMutex, Config& config);

void start();

}  // namespace Rfid

#endif  // RFID_HXX
