#ifndef RFID_HXX
#define RFID_HXX

class JsonConfig;
class SPIClass;

namespace Rfid {

void initialize(SPIClass& spi, void* spiMutex, const JsonConfig& config);

void start();

}  // namespace Rfid

#endif  // RFID_HXX
