/***************************************************************************//**
 * @file                    exercise3.cpp
 * @author                  Rafael Andrioli Bauer
 * @date                    18.11.2022
 * @matriculation number    5163344
 * @e-mail contact          abauer.rafael@gmail.com
 *
 * @brief   Exercise 3 - Interfacing External ICs
 *
 * Description:
 *
 * Pin connections:
 *
 * Theory answers: None.
 *
 * Tasks completed:
 *  Task 1
 *         [ ] Initial State                      (x/1,0 pt.)
 *         [ ] Playback PB3 pressed - 4 states/s  (x/1,0 pt.)
 *         [ ] Playback continues PB3 released    (x/1,0 pt.)
 *         [ ] Pause PB2                          (x/1,0 pt.)
 *         [ ] Fast-forward PB4 - 8 states/s      (x/1,0 pt.)
 *         [ ] Previous state PB4 released        (x/1,0 pt.)
 *         [ ] Rewind PB1 - 8 states/s            (x/1,0 pt.)
 *         [ ] Previous state PB1 released        (x/1,0 pt.)
 *         [ ] Respond to input regardless state  (x/1,0 pt.)
 *  Bonus
 *         [ ] D5 - D7 as rotating running light  (x/1.0 pt.)
 *
 *  Task 2
 *         [ ] feedback.txt                       (x/1.0 pt.)
 *
 * @note    The project was exported using CCS 12.1.0.00007
 ******************************************************************************/
#define NO_TEMPLATE_UART
#include <templateEMP.h>

#include "GPIOs.hpp"
#include "Button.hpp"
#include "ShiftRegister.hpp"
#include <utility>

using namespace Microtech;

class ExerciseLogic {
public:
    typedef void (ExerciseLogic::*ButtonStateCallback)(IOState);  ///< Type definition of callback
    typedef void (ExerciseLogic::*DirectionFunction)();  ///< Type definition of direction function
    enum class PBIndex{
        PB4 = 0,
        PB3,
        PB2,
        PB1,
    };
    constexpr ExerciseLogic(const ShiftRegisterController& registersController, const ShiftRegisterBase& registersLED, const ShiftRegisterBase& registersPB) :
            shiftRegisterController(registersController), shiftRegisterLEDs(registersLED), shiftRegisterPBs(registersPB) {
        pbCallbacks[static_cast<uint8_t>(PBIndex::PB1)] = &ExerciseLogic::PB1StateChanged;
        pbCallbacks[static_cast<uint8_t>(PBIndex::PB2)] = &ExerciseLogic::PB2StateChanged;
        pbCallbacks[static_cast<uint8_t>(PBIndex::PB3)] = &ExerciseLogic::PB3StateChanged;
        pbCallbacks[static_cast<uint8_t>(PBIndex::PB4)] = &ExerciseLogic::PB4StateChanged;
    }

    void init() const noexcept {
        shiftRegisterController.init();
        shiftRegisterLEDs.init();
        shiftRegisterPBs.init();
    }

    void changeState() noexcept {
        static uint8_t currentPBRead = 0;
        shiftRegisterController.clockOneCycle();
        ButtonStateCallback currentButtonCallback = pbCallbacks[currentPBRead % MAX_NUM_STATES];
        (this->*currentButtonCallback)(shiftRegisterPBs.getInputQDState());

        if(directionFunction != nullptr) {
            (this->*directionFunction)();
        }

        currentPBRead++;
    }

private:

    void PB1StateChanged(IOState pb1State) {
        /*
        static IOState currentStatePb1 = IOState::LOW;
        if(pb1State == currentStatePb1) {
            return;
        }
        if(pb1State == IOState::HIGH) {
            shiftRegisterLEDs.setMode(ShiftRegisterBase::Mode::SHIFT_LEFT);
            directionFunction =  &ExerciseLogic::reverse;
        } else {
            // return to previous state
        }

        currentStatePb1 = pb1State;
        */
    }

    void PB2StateChanged(IOState pb2State) {
        /*
        static IOState currentStatePb2 = IOState::LOW;
        if(pb2State == currentStatePb2) {
            return;
        }

        if(pb2State == IOState::HIGH) {
            shiftRegisterLEDs.setMode(ShiftRegisterBase::Mode::PAUSE);
            directionFunction = nullptr;
        }

        currentStatePb2 = pb2State;
        */
    }

    void PB3StateChanged(IOState pb3State) {
        /*
        static IOState currentStatePb3 = IOState::LOW;
        if(pb3State == currentStatePb3) {
            return;
        }

        if(pb3State == IOState::HIGH) {
            shiftRegisterLEDs.setMode(ShiftRegisterBase::Mode::SHIFT_RIGHT);
            directionFunction = &ExerciseLogic::forward;
        }

        currentStatePb3 = pb3State;
        */
    }

    void PB4StateChanged(IOState pb4State) {
        /*
        static IOState currentStatePb4 = IOState::LOW;
        if(pb4State == currentStatePb4) {
            return;
        }

        if(pb4State == IOState::HIGH) {
            // Speed-up
        } else {
            // return to previous state
        }
        currentStatePb4 = pb4State;
        */
    }

    void forward() {
        const uint8_t currentState = stateCount % MAX_NUM_STATES;
        if(currentState == 0) {
            shiftRegisterLEDs.outputAStateRightShift(IOState::HIGH);
        } else {
            shiftRegisterLEDs.outputAStateRightShift(IOState::LOW);
        }

        stateCount++;
    }

    void reverse() {
        const uint8_t currentState = stateCount % MAX_NUM_STATES;
        if(currentState == 0) {
            shiftRegisterLEDs.outputAStateRightShift(IOState::HIGH);
        } else {
            shiftRegisterLEDs.outputAStateRightShift(IOState::LOW);
        }
        stateCount--;
    }

    const ShiftRegisterController shiftRegisterController;
    const ShiftRegisterBase shiftRegisterLEDs;
    const ShiftRegisterBase shiftRegisterPBs;
    static const uint8_t MAX_NUM_STATES = 4;
    ButtonStateCallback pbCallbacks[MAX_NUM_STATES] = {nullptr, nullptr, nullptr, nullptr};
    DirectionFunction directionFunction = nullptr;
    uint8_t stateCount = 0;
};

int main() {
  initMSP();

  constexpr OutputHandle serialRightReg2 = GPIOs::getOutputHandle<IOPort::PORT_2, static_cast<uint8_t>(6)>();
  //ShiftRegisterBase shiftRegisterLEDs(GPIOs::getOutputHandle<IOPort::PORT_2, static_cast<uint8_t>(0)>(),
  //                                    GPIOs::getOutputHandle<IOPort::PORT_2, static_cast<uint8_t>(1)>(),
  //                                    &serialRightReg2);
  constexpr InputHandle qdOutputReg1 = GPIOs::getInputHandle<IOPort::PORT_2, static_cast<uint8_t>(7)>();
  //ShiftRegisterBase shiftRegisterPBs(GPIOs::getOutputHandle<IOPort::PORT_2, static_cast<uint8_t>(2)>(),
  //                                   GPIOs::getOutputHandle<IOPort::PORT_2, static_cast<uint8_t>(3)>(),
  //                                   &qdOutputReg1);

  //constexpr ShiftRegisterController shiftRegisterController(GPIOs::getOutputHandle<IOPort::PORT_2, static_cast<uint8_t>(4)>(),
  //                                                          GPIOs::getOutputHandle<IOPort::PORT_2, static_cast<uint8_t>(5)>());

  //ExerciseLogic exerciseLogic(shiftRegisterController, shiftRegisterLEDs, shiftRegisterPBs);

  //exerciseLogic.init();
  //shiftRegisterLEDs.init();
  //shiftRegisterLEDs.setMode(ShiftRegisterBase::Mode::SHIFT_RIGHT);
  //shiftRegisterLEDs.outputAStateRightShift(IOState::HIGH);

  //shiftRegisterController.start();

  //exerciseLogic.changeState();

  while(true) {
      __no_operation();
  }
  return 0;
}
