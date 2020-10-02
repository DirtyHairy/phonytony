#ifndef LED_STRATEGY_HXX
#define LED_STRATEGY_HXX

#include <cstdint>

#include "Gpio.hxx"

namespace Led {

class ModulationStrategy {
   public:
    enum Type { none, steady, breathe };

   public:
    virtual void tick() = 0;

    virtual uint8_t dutyCycle() = 0;
};

class OnOffStrategy {
   public:
    enum Type { none, steady, blink };

   public:
    virtual void tick() = 0;

    virtual int state() = 0;
};

class ColorStrategy {
   public:
    enum Type { none, red, green, blue };

   public:
    virtual void tick() = 0;

    virtual Gpio::LED color() = 0;
};

ModulationStrategy* resetModulationStrategy(ModulationStrategy::Type type);

OnOffStrategy* resetOnOffStrategy(OnOffStrategy::Type type);

ColorStrategy* resetColorStrategy(ColorStrategy::Type type);

}  // namespace Led

#endif  // LED_STRATEGY_HXX
