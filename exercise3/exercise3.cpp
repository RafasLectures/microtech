/***************************************************************************//**
 * @file                    exercise3.cpp
 * @author                  Rafael Andrioli Bauer
 * @date                    18.11.2022
 * @matriculation number    5163344
 * @e-mail contact          abauer.rafael@gmail.com
 *
 * @brief   Exercise 3 - Interfacing External ICs
 *
 * Description: The main starts initializing the MSP. Followed by the timer and Exercise logic
 *              The complete exercise logic, the state changes and so on was implemented in a
 *              class called Exercise Logic.
 *              The Shift registers were abstracted as well and it is described in the ShiftRegister classes
 *
 *              A timer was used, so every 5ms there is a clock tick, on every clock tick there is a clock cycle
 *              for the shift registers. At every clock cycle, one PB gets read. The control if the LEDs should change state
 *              is performed using a counter that counts how many clock ticks have happened.
 *
 *
 * Pin connections:  None
 *
 * Theory answers: None.
 *
 * Tasks completed:
 *  Task 1
 *         [x] Initial State                      (x/1,0 pt.)
 *         [x] Playback PB3 pressed - 4 states/s  (x/1,0 pt.)
 *         [x] Playback continues PB3 released    (x/1,0 pt.)
 *         [x] Pause PB2                          (x/1,0 pt.)
 *         [x] Fast-forward PB4 - 8 states/s      (x/1,0 pt.)
 *         [x] Previous state PB4 released        (x/1,0 pt.)
 *         [x] Rewind PB1 - 8 states/s            (x/1,0 pt.)
 *         [x] Previous state PB1 released        (x/1,0 pt.)
 *         [x] Respond to input regardless state  (x/1,0 pt.)
 *  Bonus
 *         [ ] D5 - D7 as rotating running light  (0.0/1.0 pt.)
 *
 *  Task 2
 *         [ ] feedback.txt                       (x/1.0 pt.)
 *
 * @note    The project was exported using CCS 12.1.0.00007
 ******************************************************************************/
#define NO_TEMPLATE_UART
#include <templateEMP.h>

#include "GPIOs.hpp"
#include "Timer.hpp"

#include "Button.hpp"
#include "ShiftRegister.hpp"
#include <utility>

using namespace Microtech;

/**
 * Class implements the exercise logic with the evaluation of the buttons and
 * manipulation of LEDs of the shift register.
 */
class ExerciseLogic {
public:
    typedef void (ExerciseLogic::*ButtonEvaluationFunction)(IOState);  ///< Type definition of Button evaluation function
    typedef void (ExerciseLogic::*DirectionFunction)();           ///< Type definition of direction function
    /**
     * Enum defining the index of each button.
     * Was created to ease us code readability
     */
    enum class PBIndex{
        PB4 = 0,
        PB3,
        PB2,
        PB1,
    };

    /**
     * Enum to say the index of which LED is on.
     */
    enum class LEDState {
        D4_ON = 0,
        D1_ON,
        D2_ON,
        D3_ON
    };

    /**
     * Constructor of the exercise logic. It it as this point where the also initializes the array with the function pointers to evaluate
     * the pushbuttons.
     *
     * @param registersController reference to the ShiftRegisterController instance
     * @param registersLED reference to the shift register responsible for controlling the LEDs
     * @param registersPB reference to the shift register responsible for controlling the PBs
     */
    constexpr ExerciseLogic(const ShiftRegisterController& registersController, const ShiftRegisterLED& registersLED, const ShiftRegisterPB& registersPB) :
            shiftRegisterController(registersController), shiftRegisterLEDs(registersLED), shiftRegisterPBs(registersPB), hasStarted(false) {
        pbEvalFunc[static_cast<uint8_t>(PBIndex::PB1)] = &ExerciseLogic::evaluatePB1;
        pbEvalFunc[static_cast<uint8_t>(PBIndex::PB2)] = &ExerciseLogic::evaluatePB2;
        pbEvalFunc[static_cast<uint8_t>(PBIndex::PB3)] = &ExerciseLogic::evaluatePB3;
        pbEvalFunc[static_cast<uint8_t>(PBIndex::PB4)] = &ExerciseLogic::evaluatePB4;
    }

    /**
     * Method that initializes the shift registers
     */
    void init() const noexcept {
        shiftRegisterController.init();
        shiftRegisterLEDs.init();
        shiftRegisterPBs.init();
    }

    /**
     * Method that performs the evaluation the evaluation of the PBs and LEDs
     * This is the method that clocks the shift registers.
     * So it uses the ShiftRegisterController to generate a clock cycle as well as evaluates
     * evaluates the PBs and LEDs after the clock cycle.
     */
    void performEvaluation() noexcept {
        // cycles the shift registers and returns what was the current PB that was read after the cycle
        uint8_t currentPBRead = cycleShiftRegister();
        // gets the pointer of the function from the array to evaluate the button
        ButtonEvaluationFunction currentButtonCallback = pbEvalFunc[currentPBRead];

        // calls the button evaluation function
        (this->*currentButtonCallback)(shiftRegisterPBs.getInputQDState());

        // Verify if there is a direction function, if there is, first, we evaluate
        // if the number of clock cycles to change LED state is already achieved.
        // If so, perform the LED state change depending.
        // Note: the direction function pointer is the one that performs the LED state change.
        if(directionFunction != nullptr) {
            if(clockTickCount < clockTicksToChangeLEDState) {
                clockTickCount++;
                shiftRegisterLEDs.setMode(ShiftRegisterBase::Mode::PAUSE);
                return;
            }
            clockTickCount = 0;
            (this->*directionFunction)();

            // This is a flag that was introduced, just in case the user presses to go
            // reverse before the forward.
            // It is necessary, because the reverse relies that the LED D1 will be on
            // Since the initial state has all LEDs OFF, and this will never happen again (according to the exercise description,
            // State 1 - 4 always has one LED on, and it goes from State 4 to 1 without going through 0,
            // we need to track if it has already started.
            if(hasStarted == false) {
                hasStarted = true;
            }
        }

    }

private:
    /**
     * This method is the one that performs the clock cycle. It is also responsible for tracking the PB
     * read counter. The PB counter never stops.
     */
    uint8_t cycleShiftRegister() noexcept {
        // The PB counter that never stops counting
        static uint8_t PBReadCounter = 0;
        // Do the PB counter mod MAX_NUM_PBs, so we know which PB we are currently reading.
        // This will always return a valid value, since up to 255 the mod is always 0, 1, 2, 3 and then the variable
        // overflows and starts again in 0
        const uint8_t currentPBRead = PBReadCounter % MAX_NUM_PBs;

        // If the current PB to read is 0, then we ask the shift register to mirror the parallel input.
        // So the value in D will be output in QD after the next cycle
        // otherwise, we just tell the shift register to shift its values right
        if(currentPBRead == 0) {
            shiftRegisterPBs.setMode(ShiftRegisterBase::Mode::MIRROR_PARALLEL);
        } else {
            shiftRegisterPBs.setMode(ShiftRegisterBase::Mode::SHIFT_RIGHT);
        }
        shiftRegisterController.clockOneCycle();

        PBReadCounter++;
        return currentPBRead;
    }

    /**
     * Method that evaluates the state of PB1
     * Whenever it is HIGH, then we start to move the LEDs on reverse at 8 states per second.
     * When it is released, it returns to the previous state.
     * @param pb1State The state of the PB 1
     */
    void evaluatePB1(IOState pb1State) {
        // Static variable to track when PB1 changes state. It just gets initialized once, since it is static
        static IOState previousStatePb1 = IOState::LOW;
        // Since we need to revert to the previous state, then we also store the pointer of the
        // direction function before the button became active.
        static DirectionFunction previousDirectionFunction;
        // If there is no state change, we do nothing.
        if(pb1State == previousStatePb1) {
            return;
        }

        if(pb1State == IOState::HIGH) {
            //shiftRegisterLEDs.setMode(ShiftRegisterBase::Mode::SHIFT_LEFT);     // Set LED shift register to shift left
            previousDirectionFunction = directionFunction;                      // Store previous direction function
            directionFunction =  &ExerciseLogic::reverse;                       // Change direction function to reverse
            clockTicksToChangeLEDState = CLK_TICKS_8_STATES_PER_SECOND;         // Change the number of clock ticks necessary to change state
        } else {
            clockTicksToChangeLEDState = CLK_TICKS_4_STATES_PER_SECOND;         // Change the number of clock ticks necessary to change state
            directionFunction = previousDirectionFunction;                      // Set previous direction function
        }

        previousStatePb1 = pb1State;
    }

    /**
     * Method that evaluates the state of PB2
     * Whenever it is HIGH, then we start to pause the LEDs
     * When it is released, we keep it paused
     * @param pb2State The state of the PB 2
     */
    void evaluatePB2(IOState pb2State) {
        static IOState previousStatePb2 = IOState::LOW;
        if(pb2State == previousStatePb2) {
            return;
        }

        if(pb2State == IOState::HIGH) {
            // Set the shift register to pause, so no matter if there are clocks,
            // the shift register will not shift the Qx outputs.
            shiftRegisterLEDs.setMode(ShiftRegisterBase::Mode::PAUSE);
            directionFunction = nullptr;                    // Set the direction function to null, so it won't be called
            clockTickCount = 0;                             // Resets the tick count, so the next time it starts again, the first LED will have the correct timing.
        }

        previousStatePb2 = pb2State;

    }

    /**
     * Method that evaluates the state of PB3
     * Whenever it is HIGH, then we start to move the LEDs forward at 4 states per second.
     * When it is released, it keeps moving forward at 4 states per second
     * @param pb3State The state of the PB 3
     */
    void evaluatePB3(IOState pb3State) {
        static IOState previousStatePb3 = IOState::LOW;
        if(pb3State == previousStatePb3) {
            return;
        }

        if(pb3State == IOState::HIGH) {
            //shiftRegisterLEDs.setMode(ShiftRegisterBase::Mode::SHIFT_RIGHT);
            clockTicksToChangeLEDState = CLK_TICKS_4_STATES_PER_SECOND;         // Change the number of clock ticks necessary to change state
            directionFunction = &ExerciseLogic::forward;
        }

        previousStatePb3 = pb3State;

    }

    /**
     * Method that evaluates the state of PB4
     * Whenever it is HIGH, then we start to move the LEDs forward at 8 states per second.
     * When it is released, it returns to the previous state.
     * @param pb3State The state of the PB 3
     */
    void evaluatePB4(IOState pb4State) {
        static IOState currentStatePb4 = IOState::LOW;
        static DirectionFunction previousDirectionFunction;
        if(pb4State == currentStatePb4) {
            return;
        }

        if(pb4State == IOState::HIGH) {
            //shiftRegisterLEDs.setMode(ShiftRegisterBase::Mode::SHIFT_RIGHT);
            clockTicksToChangeLEDState = CLK_TICKS_8_STATES_PER_SECOND;
            previousDirectionFunction = directionFunction;
            directionFunction = &ExerciseLogic::forward;
        } else {
            clockTicksToChangeLEDState = CLK_TICKS_4_STATES_PER_SECOND;
            directionFunction = previousDirectionFunction;
        }
        currentStatePb4 = pb4State;

    }

    /**
     * Method responsible for moving the LEDs forward
     */
    void forward() {
        // Calculates the current LED state. It uses the same approach as for the PBs.
        // Check the comment on cycleShiftRegister() to see about the mod.
        const uint8_t currentState = ledStateCount % MAX_NUM_LED_STATES;
        // Verifies if the current state is that the LED D4 is on.
        // If it is, it sets the mode of the LED shift register to
        // Make the Output QA as HIGH in the next right shift,
        // If not, it sets it to LOW
        if(currentState == static_cast<uint8_t>(LEDState::D4_ON)) {
            shiftRegisterLEDs.setQAStateOnRightShift(IOState::HIGH);
        } else {
            shiftRegisterLEDs.setQAStateOnRightShift(IOState::LOW);
        }

        // Sets the LED shift register to shift to the right
        shiftRegisterLEDs.setMode(ShiftRegisterBase::Mode::SHIFT_RIGHT);
        ledStateCount++;
    }

    /**
     * Method responsible for moving the LEDs backwards
     */
    void reverse() {
        // This is a verification if the LEDs already have left the initial state, where all of the LEDs are OFF.
        // An explanation about why to have the flag, is given in performEvaluation()
        if(hasStarted == false) {
           // When all the LEDs are off, it first sets the mode to shift right and the make the Output QA as HIGH in that shift.
           shiftRegisterLEDs.setMode(ShiftRegisterBase::Mode::SHIFT_RIGHT);
           shiftRegisterLEDs.setQAStateOnRightShift(IOState::HIGH);
           // make  a clock cycle
           cycleShiftRegister();
           // set the output QA back to LOW
           shiftRegisterLEDs.setQAStateOnRightShift(IOState::LOW);

           // And increase the LED state, since now the LED D1 is ON.
           ledStateCount++;
           hasStarted = true; // set the flag to true so it doesn't go here again.
        }

        // Calculates the current LED state. It uses the same approach as for the PBs.
        // Check the comment on cycleShiftRegister() to see about the mod.
        const uint8_t currentState = ledStateCount % MAX_NUM_LED_STATES;
        if(currentState == static_cast<uint8_t>(LEDState::D1_ON)) {
            // Whenever the LED D1 is ON, we have to make three shifts to the right
            // to make the LED D4 ON again.
            shiftRegisterLEDs.setMode(ShiftRegisterBase::Mode::SHIFT_RIGHT);
            cycleShiftRegister();
            cycleShiftRegister();
            cycleShiftRegister();
            // Sets the mode to PAUSE, since we are already in the correct state and we don't want to
            // change state in the next clock.
            shiftRegisterLEDs.setMode(ShiftRegisterBase::Mode::PAUSE);
        } else {
            // Otherwise, sets the mode to shift the Outputs to the left.
            shiftRegisterLEDs.setMode(ShiftRegisterBase::Mode::SHIFT_LEFT);
        }

        // Decrease the state count, since we are going backwards.
        ledStateCount--;
    }

    static const uint8_t MAX_NUM_LED_STATES = 4;                ///< Constant of how many LED states there are
    static const uint8_t MAX_NUM_PBs = 4;                       ///< Constant of how many PBs states there are
    static const uint8_t CLK_TICKS_4_STATES_PER_SECOND = 50;    ///< Constant of how clock ticks are necessary to have the rate of 4 LED states per second
    static const uint8_t CLK_TICKS_8_STATES_PER_SECOND = 25;    ///< Constant of how clock ticks are necessary to have the rate of 8 LED states per second


    const ShiftRegisterController shiftRegisterController;      ///< Instance that controls the shift registers clk and clear.
    const ShiftRegisterLED shiftRegisterLEDs;                   ///< Instance that sets the modes of the LED shift register.
    const ShiftRegisterPB shiftRegisterPBs;                     ///< Instance that sets the modes of the PB shift register.

    bool hasStarted = false;                                    ///< Flag used to know if the LEDs have already left the initial state

    /**
     * Array with the pointers of the functions that are used to evaluate each PB.
     */
    ButtonEvaluationFunction pbEvalFunc[MAX_NUM_PBs] = {nullptr, nullptr, nullptr, nullptr};
    DirectionFunction directionFunction = nullptr;              ///< Pointer of function that controls the current direction the LEDs are moving.
    uint8_t ledStateCount = 0;                                  ///< Counter of the LED states

    uint8_t clockTicksToChangeLEDState = CLK_TICKS_4_STATES_PER_SECOND;  ///< variable used on to know currently how many clock ticks are necessary in order to change LED state
    uint8_t clockTickCount = 0;                                          ///< Counter of how many clock ticks have already happened.
};

// Declared Timer0 Abstraction with CLK_DIV of 8
Timer<8> timer0;

//void clkTaskShiftRegisters() {
//    exerciseLogic.performEvaluation();
//}

int main() {
  initMSP();

  timer0.init();

  constexpr ShiftRegisterLED shiftRegisterLEDs(GPIOs::getOutputHandle<IOPort::PORT_2, static_cast<uint8_t>(0)>(),
                                      GPIOs::getOutputHandle<IOPort::PORT_2, static_cast<uint8_t>(1)>(),
                                      GPIOs::getOutputHandle<IOPort::PORT_2, static_cast<uint8_t>(6)>());

  constexpr ShiftRegisterPB shiftRegisterPBs(GPIOs::getOutputHandle<IOPort::PORT_2, static_cast<uint8_t>(2)>(),
                                     GPIOs::getOutputHandle<IOPort::PORT_2, static_cast<uint8_t>(3)>(),
                                     GPIOs::getInputHandle<IOPort::PORT_2, static_cast<uint8_t>(7)>());

  constexpr ShiftRegisterController shiftRegisterController(GPIOs::getOutputHandle<IOPort::PORT_2, static_cast<uint8_t>(4)>(),
                                                            GPIOs::getOutputHandle<IOPort::PORT_2, static_cast<uint8_t>(5)>());

  // Creates exercise logic instance and passes the shift register objects
  ExerciseLogic exerciseLogic(shiftRegisterController, shiftRegisterLEDs, shiftRegisterPBs);

  exerciseLogic.init();

  shiftRegisterController.start();

  // Creates a 5ms periodic task for performing the exercise logic.
  TaskHandler<5, std::chrono::milliseconds> clkTask(std::bind(&ExerciseLogic::performEvaluation, exerciseLogic), true);

  // registers exercise logic task to timer 0
  timer0.registerTask(clkTask);

  // globally enables the interrupts.
  __enable_interrupt();
  while(true) {
      __no_operation();
  }
  return 0;
}

// Timer0 Interruption
#pragma vector = TIMER0_A0_VECTOR
__interrupt void Timer_A_CCR0_ISR(void) {
  timer0.interruptionHappened();
}
