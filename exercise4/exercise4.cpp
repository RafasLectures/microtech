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
 * Pin connections:  U_POT <-> CON3:P1.7
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
 *    a)   [x] General Function                   (x/2,0 pt.)
 *         [x] 1st section = no LED               (x/1,0 pt.)
 *         [x] 2nd section = D1 on                (x/1,0 pt.)
 *         [x] 3rd section = D1-2 on              (x/1,0 pt.)
 *         [x] 4th section = D1-3 on              (x/1,0 pt.)
 *         [x] 5th section = D1-4 on              (x/1,0 pt.)
 *   b)
 *         [x] Detect of a chip                   (x/1,0 pt.)
 *         [x] Detect white chip                  (x/1,0 pt.)
 *         [x] Detect black chip                  (x/1,0 pt.)
 *         [x] Detect green chip                  (x/1,0 pt.)
 *         [x] Detect blue chip                   (x/1,0 pt.)
 *  Bonus
 *         [x] Detect another color               (x/1.0 pt.)
 *
 *  Task 2
 *         [ ] feedback.txt                       (x/1.0 pt.)
 *
 * @note    The project was exported using CCS 12.1.0.00007
 ******************************************************************************/
//#define NO_TEMPLATE_UART
#include <templateEMP.h>

#include "MovingAverage.hpp"
#include "GPIOs.hpp"
#include "Timer.hpp"

#include "Adc.hpp"
#include "ShiftRegister.hpp"



using namespace Microtech;

ShiftRegisterLED shiftRegisterLEDs(GPIOs::getOutputHandle<IOPort::PORT_2, static_cast<uint8_t>(4)>(),
                                   GPIOs::getOutputHandle<IOPort::PORT_2, static_cast<uint8_t>(5)>(),
                                   GPIOs::getOutputHandle<IOPort::PORT_2, static_cast<uint8_t>(0)>(),
                                   GPIOs::getOutputHandle<IOPort::PORT_2, static_cast<uint8_t>(1)>(),
                                   GPIOs::getOutputHandle<IOPort::PORT_2, static_cast<uint8_t>(6)>());


AdcHandle pot = Adc::getInstance().getAdcHandle<7>();
AdcHandle ldr = Adc::getInstance().getAdcHandle<4>();

const char* colorStr;
bool printedColor = false;
uint8_t lastPrintedColorId = 20;
uint16_t ldrValueToPrint = 0;

void clkTaskShiftRegisters() {
    const uint16_t potValue = pot.getRawValue();

    if(potValue < 204) {
        shiftRegisterLEDs.writeValue(0x00);
    }else if(potValue < 408) {
        shiftRegisterLEDs.writeValue(0x01);
    }else if(potValue < 612) {
        shiftRegisterLEDs.writeValue(0x03);
    }else if(potValue < 816) {
        shiftRegisterLEDs.writeValue(0x07);
    }else {
        shiftRegisterLEDs.writeValue(0x0F);
    }

    static SimpleMovingAverage<30> movingAverageFilter;

    const uint16_t ldrValueRaw = ldr.getRawValue();
    const uint16_t ldrValue = movingAverageFilter(ldrValueRaw);
    ldrValueToPrint = ldrValue;

    static uint8_t settleCounter = 0;
   static uint8_t lastColorId = 20;
   uint8_t colorId = 20;
   if(ldrValue > 255 && ldrValue < 271) {
       // Black
       colorStr = "Black";
       colorId = 0;
   } else if(ldrValue > 320 && ldrValue < 339) {
       // Green
       colorStr = "Green";
       colorId = 1;
   } else if(ldrValue > 341 && ldrValue < 358) {
       // Blue
       colorStr = "Blue";
       colorId = 2;
   } else if(ldrValue > 363 && ldrValue < 396) {
       // Red
       colorStr = "Red";
       colorId = 3;
   } else if(ldrValue > 495 && ldrValue < 515) {
       // White
       colorStr = "White";
       colorId = 4;
   }else if(ldrValue > 516 && ldrValue < 525) {
         // Yellow
         colorStr = "Yellow";
         colorId = 5;
   } else {
       // No chip
       colorStr = "No chip";
       colorId = 6;
   }

   if(colorId == lastColorId) {
     if(settleCounter < 20) {
         settleCounter++;
     } else {
         lastPrintedColorId = colorId;
     }
   } else {
       printedColor = false;
       lastColorId = colorId;
       settleCounter = 0;
   }

}

using Timer0 = Timer<0,8>;

int main() {
  initMSP();

  Timer0::getTimer().init();
  Adc::getInstance().init();
  shiftRegisterLEDs.init();

  constexpr OutputHandle redLed = GPIOs::getOutputHandle<IOPort::PORT_3, static_cast<uint8_t>(0)>();
  constexpr OutputHandle greenLed = GPIOs::getOutputHandle<IOPort::PORT_3, static_cast<uint8_t>(1)>();
  constexpr OutputHandle blueLed = GPIOs::getOutputHandle<IOPort::PORT_3, static_cast<uint8_t>(2)>();

  redLed.init();
  greenLed.init();
  blueLed.init();

  redLed.setState(IOState::HIGH);
  greenLed.setState(IOState::HIGH);
  blueLed.setState(IOState::HIGH);

  shiftRegisterLEDs.start();

  // Creates a 5ms periodic task for performing the exercise logic.
  TaskHandler<10, std::chrono::milliseconds> clkTask(&clkTaskShiftRegisters, true);

  // registers exercise logic task to timer 0
  Timer0::getTimer().registerTask(clkTask);

  Adc::getInstance().startConversion();
  uint8_t lastColorId = 20;

  // globally enables the interrupts.
  __enable_interrupt();
  while(true) {
      ADC10CTL0 &= ~ENC;
      while ( ADC10CTL1 & ADC10BUSY );
      ADC10CTL0 |= ENC + ADC10SC ;
      if(lastColorId != lastPrintedColorId) {
          lastColorId = lastPrintedColorId;
          serialPrintln(colorStr);
      }
      //serialPrintInt(ldrValueToPrint);
      //serialPrintln(" ");
  }
  return 0;
}
