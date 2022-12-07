/*****************************************************************************
 * @file                    exercise4.cpp
 * @author                  Rafael Andrioli Bauer
 * @date                    03.12.2022
 * @matriculation number    5163344
 * @e-mail contact          abauer.rafael@gmail.com
 *
 * @brief   Exercise 4 - Analog-To-Digital Converters
 *
 * Description: The main starts by initializing the MSP. Followed by the timer, ADC and IOs.
 *              To get the values from a specific ADC pin, one can simply request an AdcHandle from the
 *              ADC class. everytime it wants to be evaluated, one can get the raw value or a filtered value.
 *              The ADC value gets automatically updated by the usage of the DTC. For more details, please look in
 *              common/Adc.hpp.
 *
 *              Timer0 was setup with an interruption of 10ms and at every interrupt the ADC values of the potentiometer
 *              and from the LDR were evaluated in two separate functions.
 *
 *              For the bonus point the detection of YELLOW was added using a Post-it
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
 *         [x] feedback.txt                       (x/1.0 pt.)
 *
 * @note    The project was exported using CCS 12.1.0.00007
 ******************************************************************************/
#include <templateEMP.h>

#include "GPIOs.hpp"
#include "MovingAverage.hpp"
#include "Timer.hpp"

#include "Adc.hpp"
#include "ShiftRegister.hpp"

#include <chrono>

using namespace Microtech;
/**
 * Type where the color table is defined.
 */
struct ColorElement {
  uint16_t minValue;
  uint16_t maxValue;
  char* colorString;
};

ShiftRegisterLED shiftRegisterLEDs(GPIOs::getOutputHandle<IOPort::PORT_2, static_cast<uint8_t>(4)>(),
                                   GPIOs::getOutputHandle<IOPort::PORT_2, static_cast<uint8_t>(5)>(),
                                   GPIOs::getOutputHandle<IOPort::PORT_2, static_cast<uint8_t>(0)>(),
                                   GPIOs::getOutputHandle<IOPort::PORT_2, static_cast<uint8_t>(1)>(),
                                   GPIOs::getOutputHandle<IOPort::PORT_2, static_cast<uint8_t>(6)>());

// Variables used to be the transition between the interrupt and the value used in the while loop
char* colorStr = "";  // the color that will be printed.
// Only an identifier to know which color was printed last
// String comparisons usually are expensive, so this wa created to go around string comparison.
uint8_t idOfColorToPrint = 20;

/**
 * Function that evaluates the potentiometer ADC values and set the turn on the appropriate LEDs.
 */
void evaluatePotentiometer() {
  static const AdcHandle potentiometer = Adc::getInstance().getAdcHandle<7>();
  const uint16_t potValue = potentiometer.getRawValue();

  if (potValue < 204) {
    shiftRegisterLEDs.writeValue(0x00);
  } else if (potValue < 408) {
    shiftRegisterLEDs.writeValue(0x01);
  } else if (potValue < 612) {
    shiftRegisterLEDs.writeValue(0x03);
  } else if (potValue < 816) {
    shiftRegisterLEDs.writeValue(0x07);
  } else {
    shiftRegisterLEDs.writeValue(0x0F);
  }
}

/**
 * Function that evaluates the LDR ADC values and specifies which string should be printed via serial
 */
void evaluateLDR() {
  /**
   * Table of supported colors. It is structured as
   * Minimum LDR value | Maximum LDR value | Color string
   */
  static const ColorElement COLOR_TABLE[] = {
    {255, 271, "Black"}, {320, 339, "Green"}, {341, 358, "Blue"},
    {363, 396, "Red"},   {495, 515, "White"}, {516, 525, "Yellow"},
  };
  // Number of elements in the table above.
  constexpr uint16_t NUM_ELEMENTS_COLOR_TABLE = sizeof(COLOR_TABLE) / sizeof(COLOR_TABLE[0]);

  // Statically creates the handle that reads from the ADC, input 4
  static AdcHandle ldr = Adc::getInstance().getAdcHandle<4>();
  // This vatiable is used to know what was the previous detect color,
  // so we can filter for the settling time of the LDR.
  static uint8_t lastColorId = 99;  // Just initialize to some random number different than 0

  // Get the filtered value of the LDR. A Moving average over the last 30 samples.
  const uint16_t ldrValue = ldr.getFilteredValue();
  uint8_t colorId = 0;  // Variable used to loop through the color table

  // Color table loop.
  for (colorId = 0; colorId < NUM_ELEMENTS_COLOR_TABLE; colorId++) {
    // At every iteration, we check if the LDR value is within the range of a certain color defined in the
    // table. If it is, we set the colorStr, and stop the loop.
    if (ldrValue > COLOR_TABLE[colorId].minValue && ldrValue < COLOR_TABLE[colorId].maxValue) {
      colorStr = COLOR_TABLE[colorId].colorString;
      break;
    }
  }

  // Check if the colorId is equal or bigger than the Number of elements in the color table. If it is,
  // it means that we looped through every entry of the table and no match was found, therefore
  // we assume that there is no chip.
  if (colorId >= NUM_ELEMENTS_COLOR_TABLE) {
    colorStr = "No chip";
  }

  // The final section is used to create a settling of the color,
  // so it smoothens the transition from one color to another.
  // Otherwise, we are more sensible to small changes in the ldr.
  static uint8_t settleCounter = 0;
  constexpr uint8_t SETTLE_VALUE = 20;
  if (colorId == lastColorId) {
    if (settleCounter < SETTLE_VALUE) {
      settleCounter++;
    } else {
      idOfColorToPrint = colorId;
    }
  } else {
    lastColorId = colorId;
    settleCounter = 0;
  }
}

void evaluateAdcTask() {
  evaluatePotentiometer();
  evaluateLDR();
}

using Timer0 = Timer<0, 8>;

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

  // Creates a 10ms periodic task for evaluating the adc values.
  TaskHandler<10, std::chrono::milliseconds> adcTask(&evaluateAdcTask, true);

  // registers adc task to timer 0
  Timer0::getTimer().registerTask(adcTask);

  Adc::getInstance().startConversion();
  uint8_t lastPrintedColorId = 99;

  // globally enables the interrupts.
  __enable_interrupt();
  while (true) {
    ADC10CTL0 &= ~ENC;
    while (ADC10CTL1 & ADC10BUSY)
      ;                          // Waits until ADC sampling is done
    ADC10CTL0 |= ENC + ADC10SC;  // Triggers new sampling from the ADC
    if (lastPrintedColorId != idOfColorToPrint) {
      lastPrintedColorId = idOfColorToPrint;
      serialPrintln(colorStr);
    }
  }
  return 0;
}
