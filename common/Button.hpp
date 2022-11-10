/*
 * Button.hpp
 *
 *  Created on: 09.11.2022
 *      Author: Andrio01
 */

#ifndef COMMON_BUTTON_HPP_
#define COMMON_BUTTON_HPP_

#include "GPIOs.hpp"

namespace Microtech {

enum class ButtonState {
    RELEASED = 0,
    PRESSED
};

template <IOPort port, uint8_t Pin>
class Button {
public:
    constexpr Button(bool invertedLogic) : inputPin(), invertedLogic(invertedLogic) {}

    void init() {
        setState(evaluateButtonState(inputPin.getState()));
    }

    ButtonState getState() noexcept {
        return state;
    }

    void setState(ButtonState newState) {
        if(state == newState) {
            return;
        }
        state = newState;
        if(stateCallback != nullptr) {
            stateCallback(state);
        }
    }

    void performDebounce() noexcept {
        static const uint16_t DEBOUNCE_MAX = 5;

        const IOState currentPinState = inputPin.getState();

        if(previousPinState == currentPinState) {
            if(debounceCounter < DEBOUNCE_MAX) {
                debounceCounter++;
            } else {
                setState(evaluateButtonState(currentPinState));
            }
        } else {
            previousPinState = currentPinState;
            debounceCounter = 0;
        }
    }

    void enablePinInterrupt() {
        inputPin.enableInterrupt();
    }

    void registerStateChangeCallback(void(*callbackPtr)(ButtonState)) {
        stateCallback = callbackPtr;
    }


    ButtonState evaluateButtonState(const IOState pinState) noexcept {
        if(invertedLogic) {
            switch(pinState) {
            case IOState::LOW: return ButtonState::PRESSED;
            default:
            case IOState::HIGH: return ButtonState::RELEASED;
            };
        }else {
            switch(inputPin.getState()) {
            case IOState::LOW: return ButtonState::RELEASED;
            default:
            case IOState::HIGH: return ButtonState::PRESSED;
            };
        }
    }

private:
    typename GPIOs<port>::InputHandle<Pin> inputPin;
    bool invertedLogic;
    ButtonState state;
    void (*stateCallback)(ButtonState) = nullptr;
    IOState previousPinState;
    uint16_t debounceCounter = 0;
};

} /* namespace Microtech */

#endif /* COMMON_BUTTON_HPP_ */
