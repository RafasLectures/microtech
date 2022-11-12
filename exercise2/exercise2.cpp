/***************************************************************************//**
 * @file                    exercise2.cpp
 * @author                  Rafael Andrioli Bauer
 * @date                    09.11.2022
 * @matriculation number    5163344
 * @e-mail contact          abauer.rafael@gmail.com
 *
 * @brief   Exercise 2 - Digital I/O and Interrupts
 *
 * Description: The initialization of the IOs is performed before main, since
 *              the declaration of the IOs (OutputHandle and Buttons) are outside void main().
 *              
 *              In the beggining of main(), the system clocks and watchdog are initialized via the
 *              initMSP() call followed by the callback registration of the methods
 *              "listenPB5() and listenPB6()" to the buttons PB5 and PB6 respectively. This means
 *              that whenever PB5 changes its state listenPB5() gets called. The same happens
 *              for PB6 but then calling listenPB6().
 *
 *              Both buttons are initialized with the call pbx.init() followed by the timer0
 *              initialization.
 *
 *              A periodic task of 5ms is used to trigger the debounce of the buttons. The
 *              debounce logic is performed by the button itself. 
 *              
 *              The evaluation of the LEDs state is done by the StateMachine class and it is
 *              triggered only when there is a button state change.
 *
 * Pin connections: PB5 <-> CON3:P1.3
 *                  PB6 <-> CON3:P1.4
 *                  D6  <-> CON3:P1.5
 *                  D5  <-> CON3:P1.6
 *                  D7  <-> CON3:P1.0
 *                  D9  <-> CON3:P1.7
 *
 * Theory answers: None.
 *
 * Tasks completed:
 *  Task 1
 *      a) [x] State 1                       (x/1,5 pt.)
 *         [x] State 2                       (x/1,5 pt.)
 *         [x] State 3                       (x/1,5 pt.)
 *         [x] State 4                       (x/1,5 pt.)
 *      b) [x] Separate function D6 and D9   (x/1.0 pt.)
 *      c) [x] PB5 interrupt                 (x/2,0 pt.)
 *
 *  Task 2
 *         [x] feedback.txt                  (x/1.0 pt.)
 *
 * @note    The project was exported using CCS 12.1.0.00007
 ******************************************************************************/

//#define NO_TEMPLATE_UART
//#define polling
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
GPIOs<IOPort::PORT_1>::OutputHandle<0> blueLedD7;
GPIOs<IOPort::PORT_1>::OutputHandle<5> redLedD6;
GPIOs<IOPort::PORT_1>::OutputHandle<6> greenLedD5;
GPIOs<IOPort::PORT_1>::OutputHandle<7> yellowLedD9;

// Declare the two buttons
Button<IOPort::PORT_1, 3> pb5(true);
Button<IOPort::PORT_1, 4> pb6(true);

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

        // Calls method that controls D6 and D9.
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
            redLedD6.setState(IOState::HIGH);
            yellowLedD9.setState(IOState::LOW);
            // Puts count to 0 so timer starts counting
            countTimeD6On = 0;
        } else{
            redLedD6.setState(IOState::LOW);
            yellowLedD9.setState(IOState::HIGH);
        }
    }

    /**
     * Flag used by the timer to know if it has to keep the LED OFF or ON.
     * Since the timer is 5ms, then 50 times would be 250ms.
     * Initialize more than 250ms (51 times) so the timer doesn't change it.
     */
    uint16_t countTimeD6On = 51;
    ButtonState statePB5 = ButtonState::RELEASED;   ///< Holds the state of the button PB5
    ButtonState statePB6 = ButtonState::RELEASED;   ///< Holds the state of the button PB6
};

StateMachine stateMachine;      ///< Declaration of the state machine.

/**
 * Method is a callback of the button PB5. So whenever the button changes
 * state, this method gets called.
 *
 *
 * @note Actually the state machine should be listening for the events, but
 *       I didn't find a better way to make set a method of the StateMachine class
 *       to listen to the events, therefore I simply creted this static function.
 *       There must be a better way of doing. For now just did like this so it works
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
 * and have the option to have wake-up tasks, so one could say that want to be awake in 250ms.
 *
 */
void pollingTaskCallback() {
    // Whenever the PB5 has to be evaluated using polling, the performDebounce is called.
#ifdef polling
    pb5.performDebounce();
#endif
    pb6.performDebounce();

    // Verify if the counter of how long D6 is on is less than 50. Since the polling happens every
    // 5 ms, this function has to be called 250ms/5ms = 50
    if(stateMachine.countTimeD6On < 50) {
        stateMachine.countTimeD6On++;      // increment counter value
    } else{
        redLedD6.setState(IOState::LOW);
        yellowLedD9.setState(IOState::HIGH);
    }

}

void main() {
    initMSP();
    
    // Registers static functions as callbacks of the button instances
    pb5.registerStateChangeCallback(&listenerPB5);
    pb6.registerStateChangeCallback(&listenerPB6);

    // Initialized buttons
    pb5.init();
    pb6.init();

    // Initialize the timer0
    timer0.init();

    // Creates a 5ms periodic task for polling.
    TaskHandler<5, std::chrono::milliseconds> pollingTask(&pollingTaskCallback, true);

    // registers polling task to timer 0
    timer0.registerTask(pollingTask);
    
    // If pb5 should not be performed with polling, then we enable the input pin interrupt of the button 
#ifndef polling
    pb5.enablePinInterrupt();
#endif

    // globally enables the interrupts.
    __enable_interrupt(); 
    while(1) {}  // infinite loop so program doesn't return
}

// Not sure how to encapsulate the interruptions yet.

//Timer0 Interruption
#pragma vector = TIMER0_A0_VECTOR
__interrupt void Timer_A_CCR0_ISR(void) {
   timer0.interruptionHappened();
}

#ifndef polling
// Port 1 interrupt vector
# pragma vector = PORT1_VECTOR
__interrupt void Port_1_ISR ( void ) {
    // Gets the current edge detection direction
    // if 1 = HIGH -> LOW;  0 = LOW -> HIGH
    volatile uint8_t registerVal = getRegisterBits(P1IES, static_cast<uint8_t>(BIT3), static_cast<uint8_t>(3));
    
    // Evaluates current button state based on register edge detection value.
    // The XOR is done because if the edge detection is 1 => HIGH -> LOW; then it means that
    // the IOState must be LOW, therefore the result of 1 XOR 1 is 0.
    // If the edge detection is 0 => LOW -> HIGH, then the IOState must be HIGH, so the 
    // result of 0 XOR 1 is 1.
    ButtonState pb5CurrentState = pb5.evaluateButtonState((IOState)(registerVal ^ 0x01)); 
    // Sets the state of th button based on the button state evaluation.
    // If the button state is different, internally the button calls its callback which calls
    // the state machine evaluation.
    pb5.setState(pb5CurrentState);
    
    // toggles direction edge between high/low and low/high so we can catch when the
    // button go to the other state
    toggleRegisterBits(P1IES, static_cast<uint8_t>(BIT3));
    
    resetRegisterBits(P1IFG, static_cast<uint8_t>(BIT3));  // clear interrupt flag
}
#endif
