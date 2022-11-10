/***************************************************************************//**
 * @file                    exercise2.cpp
 * @author                  Rafael Andrioli Bauer
 * @date                    09.11.2022
 * @matriculation number    5163344
 * @e-mail contact          abauer.rafael@gmail.com
 *
 * @brief   Exercise 2 - Digital I/O and Interrupts
 *
 * Description: write about your code
 *
 * Pin connections: mention all the pin connections
 *
 * Theory answers: if required. With question number.
 *
 * Tasks completed:
 *  Task 1
 *      a) [x] State 1                       (x/1,5 pt.)
 *         [x] State 2                       (x/1,5 pt.)
 *         [x] State 3                       (x/1,5 pt.)
 *         [x] State 4                       (x/1,5 pt.)
 *      b) [x] Separate function D6 and D9   (x/1.0 pt.)
 *      c) [ ] PB5 interrupt                 (x/2,0 pt.)
 *
 *  Task 2
           [ ] feedback.txt                  (0.0/1.0 pt.)
 *
 * @note    The project was exported using CCS 12.1.0.00007
 ******************************************************************************/

//#define NO_TEMPLATE_UART
#include <templateEMP.h>

#include "helpers"
#include "GPIOs.hpp"
#include "Timer.hpp"
#include "Button.hpp"


using namespace Microtech;

// Declared Timer0 Abstraction with CLK_DIV of 8
Timer<8> timer0;

// Declaration of IOs
// P1.0 as output -> named blueLedD7
// P1.5 as output -> named redLedD6
// P1.6 as output -> named greenLedD5
// P1.7 as output -> named yellowLedD9
GPIOs::OutputHandle<GPIOs::Port::PORT_1> blueLedD7 = GPIOs::getOutputHandle<GPIOs::Port::PORT_1>(static_cast<uint8_t>(BIT0));
GPIOs::OutputHandle<GPIOs::Port::PORT_1> redLedD6 = GPIOs::getOutputHandle<GPIOs::Port::PORT_1>(static_cast<uint8_t>(BIT5));
GPIOs::OutputHandle<GPIOs::Port::PORT_1> greenLedD5 = GPIOs::getOutputHandle<GPIOs::Port::PORT_1>(static_cast<uint8_t>(BIT6));
GPIOs::OutputHandle<GPIOs::Port::PORT_1> yellowLedD9 = GPIOs::getOutputHandle<GPIOs::Port::PORT_1>(static_cast<uint8_t>(BIT7));

Button<GPIOs::Port::PORT_1> pb5(true, static_cast<uint8_t>(BIT3));
Button<GPIOs::Port::PORT_1> pb6(true, static_cast<uint8_t>(BIT4));

/**
 * Class to bundle state machine specification
 */
class StateMachine {
public:
    /**
     * Method to evaluate the state machine.
     * Whenever any of the button states change. This function should be called.
     */
    void evaluate(){
        // State of greenLedD5 = PB6 & (PB5 ^ PB6)
        // State of blueLedD7 = PB6 & !(PB5 ^ PB6)
        greenLedD5.setState(static_cast<bool>(statePB6) & (static_cast<bool>(statePB5) ^ static_cast<bool>(statePB6)));
        blueLedD7.setState(static_cast<bool>(statePB6) & !(static_cast<bool>(statePB5) ^ static_cast<bool>(statePB6)));

        controlD6AndD9();
    }

    /**
     * Method created to fulfill Task 1.b)
     *
     * State of redLedD6 = always OFF with exception of 250 ms when PB5 & !PB6
     * State of yellowLedD9 = !redLedD6
     */
    void controlD6AndD9() {
        // Evaluation of the redLedD6 and yellowLedD9
        if(statePB5 == ButtonState::PRESSED && statePB6 == ButtonState::RELEASED) {
            redLedD6.setState(GPIOs::IOState::HIGH);
            yellowLedD9.setState(GPIOs::IOState::LOW);
            // Puts count to 0 so timer starts counting
            countTimeD6On = 0;
        } else{
            redLedD6.setState(GPIOs::IOState::LOW);
            yellowLedD9.setState(GPIOs::IOState::HIGH);
        }
    }

    /**
     * Flag used by the timer to know if it has to keep the LED OFF or ON.
     * Since the timer is 5ms, then 50 times would be 250ms.
     * Initialize more than 250ms (51 times) so the timer doesn't change it.
     */
    uint16_t countTimeD6On = 51;
    ButtonState statePB5 = ButtonState::RELEASED;   ///< Holds the sate of the button PB5
    ButtonState statePB6 = ButtonState::RELEASED;   ///< Holds the sate of the button PB6
};

StateMachine stateMachine;      ///< Declaration of the state machine.

/**
 * There must be a better way of doing. For now just did like this
 * so it works
 */
void listenerPB5(ButtonState newState){
    stateMachine.statePB5 = newState;
    stateMachine.evaluate();
}

/**
 * This function is listening to the state changes of the button PB6.
 * It has subscribed as a listener in the call pb6.registerStateChangeCallback(&listenerPB6);
 * So this is evaluated only when the button PB6 changes state.
 */
void listenerPB6(ButtonState newState){
    stateMachine.statePB6 = newState;
    stateMachine.evaluate();
}


/**
 * This function is the one that gets called by the timer.
 * It is callback of the pollingTask, declared in the main.
 *
 * It is also being used to track the 250ms that the D6 and D9 have to remain ON and OFF.
 *
 * The same task is being used because the timer only supports one task at the moment.
 * The initial idea was to be able to have up to 5 tasks per timer
 * and have the optional to have wake-up tasks, so one could say that want to be awaken in 250ms.
 *
 */
void pollingTaskCallback() {
#ifdef polling
    pb5.performDebounce();
#endif
    pb6.performDebounce();

    if(stateMachine.countTimeD6On < 50) {
        stateMachine.countTimeD6On++;
    } else{
        redLedD6.setState(GPIOs::IOState::LOW);
        yellowLedD9.setState(GPIOs::IOState::HIGH);
    }

}



void main() {
    initMSP();

    pb5.registerStateChangeCallback(&listenerPB5);
    pb6.registerStateChangeCallback(&listenerPB6);

    timer0.init();

    TaskHandler<5, std::chrono::milliseconds> pollingTask(&pollingTaskCallback, true);

    timer0.registerTask(pollingTask);
    __enable_interrupt();
    while(1) {}
}

// Not sure how t encapsulate the interruption yet.
//Timer0 Interruption
#pragma vector = TIMER0_A0_VECTOR
__interrupt void Timer_A_CCR0_ISR(void) {
   timer0.interruptionHappened();
}


