/***************************************************************************//**
 * @file                    exercise4.cpp
 * @author                  Rafael Andrioli Bauer
 * @date                    03.12.2022
 * @matriculation number    5163344
 * @e-mail contact          abauer.rafael@gmail.com
 *
 * @brief   Exercise 4 - Analog-To-Digital Converters
 *
 * Description:
 *
 * Pin connections:  P1 <-> CON3:P1.7
 *                   LDR <-> CON3:P1.4
 *                   Red D6 <-> CON3:P3.0
 *                   Green D5 <-> CON3:P3.1
 *                   Blue D7 <-> COM3:P3.2
 *                   JP2 to COL
 *
 * Theory answers: None.
 *
 * Tasks completed:
 *  Task 1
 *    a)   [ ] General Function                   (x/2,0 pt.)
 *         [ ] 1st section = no LED               (x/1,0 pt.)
 *         [ ] 2nd section = D1 on                (x/1,0 pt.)
 *         [ ] 3rd section = D1-2 on              (x/1,0 pt.)
 *         [ ] 4th section = D1-3 on              (x/1,0 pt.)
 *         [ ] 5th section = D1-4 on              (x/1,0 pt.)
 *   b)
 *         [ ] Detect of a chip                   (x/1,0 pt.)
 *         [ ] Detect white chip                  (x/1,0 pt.)
 *         [ ] Detect black chip                  (x/1,0 pt.)
 *         [ ] Detect green chip                  (x/1,0 pt.)
 *         [ ] Detect blue chip                   (x/1,0 pt.)
 *  Bonus
 *         [ ] Detect another color               (x/1.0 pt.)
 *
 *  Task 2
 *         [ ] feedback.txt                       (x/1.0 pt.)
 *
 * @note    The project was exported using CCS 12.1.0.00007
 ******************************************************************************/
//#define NO_TEMPLATE_UART
#include <templateEMP.h>

#include "GPIOs.hpp"
#include "Timer.hpp"

#include "Adc.hpp"
#include "ShiftRegister.hpp"

using namespace Microtech;

constexpr ShiftRegisterLED shiftRegisterLEDs(GPIOs::getOutputHandle<IOPort::PORT_2, static_cast<uint8_t>(0)>(),
                                    GPIOs::getOutputHandle<IOPort::PORT_2, static_cast<uint8_t>(1)>(),
                                    GPIOs::getOutputHandle<IOPort::PORT_2, static_cast<uint8_t>(6)>());


constexpr ShiftRegisterController shiftRegisterController(GPIOs::getOutputHandle<IOPort::PORT_2, static_cast<uint8_t>(4)>(),
                                                          GPIOs::getOutputHandle<IOPort::PORT_2, static_cast<uint8_t>(5)>());

AdcHandle pot = Adc::getInstance().getAdcHandle<7>();

void clkTaskShiftRegisters() {
  //serialPrint("Pot value is %i\n");
}

using Timer0 = Timer<0,8>;

int main() {
  initMSP();

  Timer0::getTimer().init();
  Adc::getInstance().init();

  shiftRegisterController.start();

  // Creates a 5ms periodic task for performing the exercise logic.
  TaskHandler<5, std::chrono::milliseconds> clkTask(&clkTaskShiftRegisters, true);

  // registers exercise logic task to timer 0
  Timer0::getTimer().registerTask(clkTask);

  Adc::getInstance().startConversion();

  // globally enables the interrupts.
  __enable_interrupt();
  while(true) {
      __no_operation();
  }
  return 0;
}
