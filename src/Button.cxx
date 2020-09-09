#include "Button.hxx"

#include "config.h"

Button::Button(uint8_t pinMask, handlerT handler) : pinMask(pinMask), stdHandler(handler) {}

Button::Button(uint8_t pinMask, uint32_t repeatDelay, handlerT handler)
    : pinMask(pinMask), repeatDelay(repeatDelay), stdHandler(handler) {}

Button::Button(uint8_t pinMask, handlerT handler, uint32_t longPressDelay, handlerT longPressHandler,
               handlerT longPressUpHandler)
    : pinMask(pinMask),
      longPressDelay(longPressDelay),
      stdHandler(handler),
      longPressHandler(longPressHandler),
      longPressUpHandler(longPressUpHandler) {}

void Button::updateState(uint8_t pins, uint64_t timestamp) {
    const State newState = pins & pinMask ? State::down : State::up;

    if (newState == state || (state == State::poweron && newState == State::up)) return;

    state = newState;
    lastTimestamp = timestamp;
    pending = true;
}

void Button::notify(uint64_t timestamp) {
    uint32_t delta = timestamp - lastTimestamp;

    if (pending && delta >= DEBOUNCE_DELAY) {
        pending = false;
        lastTimestamp = timestamp;

        switch (state) {
            case State::down:
                longPress = false;
                repeat = 0;
                break;

            case State::up:
                if (!longPress && repeat == 0 && stdHandler) stdHandler();
                if (longPress && longPressUpHandler) longPressUpHandler();
                break;

            case State::poweron:
                break;
        }

        return;
    }

    if (!pending && state == State::down && (delta - repeat * repeatDelay) >= repeatDelay && stdHandler) {
        stdHandler();
        repeat++;
    }

    if (!pending && state == State::down && delta >= longPressDelay && !longPress && longPressHandler) {
        longPress = true;

        longPressHandler();
    }
}

uint32_t Button::delayToNextNotification(uint64_t timestamp) const {
    uint32_t delta = timestamp - lastTimestamp;

    if (pending) {
        return DEBOUNCE_DELAY > delta ? DEBOUNCE_DELAY - delta : 0;
    }

    if (state == State::down && repeatDelay < NEVER) {
        return repeatDelay > (delta % repeatDelay) ? repeatDelay - (delta % repeatDelay) : 0;
    }

    if (state == State::down && !longPress && longPressHandler) {
        return longPressDelay > delta ? longPressDelay - delta : 0;
    }

    return NEVER;
}
