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

template <GPIOs::Port port>
class Button {
public:
    constexpr Button(bool invertedLogic, uint8_t inputPin) : inputPin(inputPin), invertedLogic(invertedLogic) {}

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

        const GPIOs::IOState currentPinState = inputPin.getState();

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

    void registerStateChangeCallback(void(*callbackPtr)(ButtonState)) {
        stateCallback = callbackPtr;
    }

private:
    ButtonState evaluateButtonState(const GPIOs::IOState pinState) noexcept {
        if(invertedLogic) {
            switch(pinState) {
            case GPIOs::IOState::LOW: return ButtonState::PRESSED;
            default:
            case GPIOs::IOState::HIGH: return ButtonState::RELEASED;
            }
        }else {
            switch(inputPin.getState()) {
            case GPIOs::IOState::LOW: return ButtonState::RELEASED;
            default:
            case GPIOs::IOState::HIGH: return ButtonState::PRESSED;
            }
        }
    }
    GPIOs::InputHandle<port> inputPin;
    bool invertedLogic;
    ButtonState state;
    void (*stateCallback)(ButtonState) = nullptr;
    GPIOs::IOState previousPinState;
    uint16_t debounceCounter = 0;
};

} /* namespace Microtech */

#endif /* COMMON_BUTTON_HPP_ */
