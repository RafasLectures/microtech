/*******************************************************************************
 * @file                    Button.hpp
 * @author                  Rafael Andrioli Bauer
 * @date                    09.11.2022
 * @matriculation number    5163344
 * @e-mail contact          abauer.rafael@gmail.com
 *
 * @brief   Header contains an abstraction for a Button
 *
 * @note    The project was exported using CCS 12.1.0.00007
 ******************************************************************************/

#ifndef COMMON_BUTTON_HPP_
#define COMMON_BUTTON_HPP_

#include "GPIOs.hpp"

namespace Microtech {

/**
 * Enum that contains the possible states of a Button
 */
enum class ButtonState {
    RELEASED = 0,           ///< The button is not being pressed.
    PRESSED                 ///< The button is being pressed.
};

/**
 * Class represents a button
 * One can define to which port and Pin this button is connected to
 * and whether the input logic is inverted or not.
 *
 * Appart from that, the its debounce is performed by the button and one can
 * also subscribe to the buttons state changes instead of polling the button state
 * all the time.
 *
 * @tparam port Port of the pin that the button is connected to.
 * @tparam Pin Pin that the button is connected to.
 *
 * @note it could be nice to find a way to remove these template parameters from
 * the button and let someone set the pin with a function.
 *
 */
template <IOPort port, uint8_t Pin>
class Button {
public:
    /**
     * Class constructor.
     * One can say if it is inverted logic:
     * 0 = ButtonState::PRESSED
     * 1 = ButtonState::RELEASED
     */
    constexpr Button(bool invertedLogic) : inputPin(), invertedLogic(invertedLogic) {}

    /**
     * Initialize the button. Gets state of the input pin and sets it as its state.
     */
    void init() noexcept {
        setState(evaluateButtonState(inputPin.getState()));
    }

    /**
     * Method to return the buttons state
     * @returns The button state
     */
    ButtonState getState() noexcept {
        return state;
    }

    /**
     * Method to set the state of the button. Whenever a new state is set
     * it is also responsible for calling the button's callback
     *
     * @param newState The new state of the button
     *
     * @note In theory this method should be private, but for now
     *       I could not find a way to have the interrupts encapsulated,
     *       therefore, this is public.
     */
    void setState(ButtonState newState) noexcept {
        // If the newState is the same, there is no need to do
        // anything.
        if(state == newState) {
            return;
        }
        state = newState;

        // Make sure the callback pointer is not null before
        // calling it so there is no invalid memory access
        if(stateCallback != nullptr) {
            stateCallback(state);   // calls the state callback
        }
    }

    /**
     * Method to perform the button debounce
     * Usually this is will be a function called by another periodic function
     * to constantly perform the pulling of the button state and then perform
     * debounce.
     */
    void performDebounce() noexcept {
        // The numbers of time the pin has to be in the same state in order
        // for the button to change state
        static const uint16_t DEBOUNCE_MAX = 5;

        // Polls current state from the inputPin
        const IOState currentPinState = inputPin.getState();

        // Makes sure that the current state of the pin didn't change from
        // one iteration to another.
        if(previousPinState == currentPinState) {
            if(debounceCounter < DEBOUNCE_MAX) {
                debounceCounter++;
            } else {
                // If the debounce counter is exceeded, we set the newButton State
                setState(evaluateButtonState(currentPinState));
            }
        } else {
            // If the pin state changed, then we reset the counter and set the previous
            // pin state as the current state, so we can perform the debounce in
            // the next iteration
            previousPinState = currentPinState;
            debounceCounter = 0;
        }
    }

    /**
     *  Method to enable the interrupt on the pin that the button is connected to.
     */
    void enablePinInterrupt() {
        inputPin.enableInterrupt();
    }

    /**
     * Method to register a callback to the the button state, so whenever the button
     * changes state, the callback gets called.
     *
     * @param callbackPtr pointer to the callback function
     */
    void registerStateChangeCallback(void(*callbackPtr)(ButtonState)) noexcept {
        stateCallback = callbackPtr;
    }


    /**
     * Method to set the state of the button. Whenever a new state is set
     * it is also responsible for calling the button's callback
     *
     * @param newState The new state of the button
     * @return the button state based on the input state
     *
     * @note In theory this method should be private, but for now
     *       I could not find a way to have the interrupts encapsulated,
     *       therefore, this is public.
     */
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
    typename GPIOs<port>::InputHandle<Pin> inputPin; ///< Input pin of the button
    /**
     * If the logic of the button is inverted. Meaning that:
     * isInverted true:
     *      IOState::LOW = ButtonState::PRESSED and IOState::HIGH = ButtonState::RELEASED
     */
    const bool invertedLogic;
    ButtonState state;                              ///< Current state of the button
    void (*stateCallback)(ButtonState) = nullptr;   ///< Function pointer to the state callback
    /**
     * It holds the previous inputPin state. Used by the debounce function
     */
    IOState previousPinState;
    uint16_t debounceCounter = 0;                   ///< Holds the counter for the debounce.
};

} /* namespace Microtech */

#endif /* COMMON_BUTTON_HPP_ */
