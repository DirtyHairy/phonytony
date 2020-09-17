#ifndef GPIO_HXX
#define GPIO_HXX

#include <cstdint>

#include "config.h"

class SPIClass;

namespace Gpio {

enum class LED : uint8_t { red = PIN_LED_RED_MCP, green = PIN_LED_GREEN_MCP, blue = PIN_LED_BLUE_MCP, none = 0xff };

void initialize(SPIClass& spi, void* spiMutex);

void start();

void enableAmp();

void disableAmp();

uint8_t readConfigSwitches();

uint8_t readTP5400Status();

void enableLed(LED led);

void prepareSleep();

bool silentStart();

}  // namespace Gpio

#endif  // GPIO_HXX
