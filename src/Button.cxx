#include "Button.hxx"

#include "config.h"

Button::Button(uint8_t pinMask, handlerT handler) : pinMask(pinMask), stdHandler(handler) {}

Button::Button(uint8_t pinMask, handlerT handler, uint32_t longPressDelay, handlerT longPressHandler)
    : pinMask(pinMask), longPressDelay(longPressDelay), stdHandler(handler), longPressHandler(longPressHandler) {}

void Button::updateState(uint8_t pins) {
    const State newState = pins & pinMask ? State::down : State::up;

    if (newState == state) return;

    state = newState;
    pending = true;
}

void Button::notify(uint64_t timestamp) {
    uint32_t delta = timestamp - lastNotification;
    lastNotification = timestamp;

    if (pending && delta >= DEBOUNCE_DELAY) {
        pending = false;

        switch (state) {
            case State::down:
                longPress = false;
                break;

            case State::up:
                if (!longPress && stdHandler) stdHandler();
                break;
        }

        return;
    }

    if (!pending && state == State::down && delta >= longPressDelay && !longPress && longPressHandler) {
        longPress = true;

        longPressHandler();
    }
}

uint32_t Button::delayToNextNotification(uint64_t timestamp) const {
    uint32_t delta = timestamp - lastNotification;

    if (pending) {
        return DEBOUNCE_DELAY > delta ? DEBOUNCE_DELAY - delta : 0;
    }

    if (state == State::down && !longPress && longPressHandler) {
        return longPressDelay > delta ? longPressDelay - delta : 0;
    }

    return NEVER;
}
