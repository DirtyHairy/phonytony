#ifndef SIGNAL_HXX
#define SIGNAL_HXX

#include <cstdint>

class Signal {
   public:
    enum Type : uint8_t { commandReceived = 0, error = 1 };

    struct Step {
        uint8_t note;
        uint32_t duration;
        int16_t amplitude;
    };

   public:
    Signal() = default;

    void start(Type type);

    bool isActive() const { return active; }

    uint32_t play(int16_t* buffer, uint32_t count);

   private:
    bool active{false};

    const Step* step;

    uint32_t currentSampleIndex;
};

#endif  // SIGNAL_HXX
