#ifndef RFID_HXX
#define RFID_HXX

class Config;
class SPIClass;

namespace Rfid {

void initialize(SPIClass& spi, void* spiMutex, Config& config);

void start();

void stop();

}  // namespace Rfid

#endif  // RFID_HXX
