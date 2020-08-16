#ifndef BUTTON_HXX
#define BUTTON_HXX

#include <cstdint>
#include <functional>

class Button {
   public:
    static constexpr uint32_t NEVER = 0xffffffff;

    using handlerT = std::function<void()>;

   public:
    Button(uint8_t pinMask, handlerT handler);
    Button(uint8_t pinMask, handlerT handler, uint32_t longPressDelay, handlerT longPressHandler);

    void updateState(uint8_t pins);

    void notify(uint64_t timestamp);
    uint32_t delayToNextNotification(uint64_t timestamp) const;

   private:
    enum class State { up, down };

   private:
    const uint8_t pinMask;

    State state{State::up};
    bool pending{false};
    bool longPress{false};

    const uint32_t longPressDelay{NEVER};
    uint64_t lastNotification{0};

    handlerT stdHandler;
    handlerT longPressHandler;
};

#endif  // BUTTON_HXX
