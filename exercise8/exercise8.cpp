/*****************************************************************************
 * @file                    exercise8.cpp
 * @author                  Rafael Andrioli Bauer
 * @date                    15.01.2023
 * @matriculation number    5163344
 * @e-mail contact          abauer.rafael@gmail.com
 *
 * @brief   Exercise 9 - Project Option 1 - Oscilloscope and Signal generator
 *
 * Description:     The software includes the functionality to generate sinusoidal,
 *                  trapezoidal, and rectangular signals with a maximum frequency of 5Hz,
 *                  a minimum frequency of 0.5Hz, and a frequency step of 0.5Hz.
 *                  The amplitude of the signals can also be adjusted with an amplitude step of 0.05.
 *
 *                  The oscilloscope prints new values every 20ms.
 *
 * Pin connections:
 *       CON3:P1.0 <-> DAC_OUT
 *       JP5 -> INT
 *       CON3:P3.6 <-> DAC_IN
 *       CON3:P1.5 <-> PB5
 *       CON3:P1.6 <-> PB6
 *
 * Theory answers: None
 *
 * Tasks completed:
 *      Implemented project
 *
 * @note    The project was exported using CCS 12.1.0.00007
 ******************************************************************************/
#include <templateEMP.h>

#include "Adc.hpp"
#include "Button.hpp"
#include "GPIOs.hpp"
#include "Pwm.hpp"
#include "ShiftRegister.hpp"
#include "SignalGenerator.hpp"
#include "Timer.hpp"

using namespace Microtech;

// Buttons
Button btnDecreaseAmplitude(GPIOs::getInputHandle<IOPort::PORT_1, static_cast<uint8_t>(5)>(), true);
Button btnIncreaseAmplitude(GPIOs::getInputHandle<IOPort::PORT_1, static_cast<uint8_t>(6)>(), true);

// Oscilloscope channel
AdcHandle adcCH1 = Adc::getInstance().getAdcHandle<0>();

// Signal Generator with sampling frequency of 50Hz = every 20ms.
SignalGenerator signalGenerator(50);
// Create handle of PWM for pin 6 from port 3
Pwm DAC_IN(GPIOs::getOutputHandle<IOPort::PORT_3, static_cast<uint8_t>(6)>());

constexpr ShiftRegisterPB pb1to4(GPIOs::getOutputHandle<IOPort::PORT_2, static_cast<uint8_t>(4)>(),
                                 GPIOs::getOutputHandle<IOPort::PORT_2, static_cast<uint8_t>(5)>(),
                                 GPIOs::getOutputHandle<IOPort::PORT_2, static_cast<uint8_t>(2)>(),
                                 GPIOs::getOutputHandle<IOPort::PORT_2, static_cast<uint8_t>(3)>(),
                                 GPIOs::getInputHandle<IOPort::PORT_2, static_cast<uint8_t>(7)>());

/**
 * @brief Decreases the value of the given counter.
 * If the counter is already at 0, it will stay at 0.
 * @param[in, out] counter The counter that will be decreased.
 */
void decreaseCounter(uint8_t &counter) {
  if (counter > 0) {
    counter--;
  }
}

/**
 * @brief Interrupt service routine called by the timer
 * It will read the values of the oscilloscope and send it to the serial port
 * It also handles the debouncing of the buttons and updates the signal generator's
 * amplitude and frequency based on user input.
 */
void timerInterrupt() {
  _iq15 nextDatapoint = signalGenerator.getNextDatapoint();
  // Print values of Oscilloscope
  serialPrintInt(adcCH1.getRawValue());
  //serialPrint(" ");
  //serialPrintInt(_IQ15int(nextDatapoint)); // For debugging purposes
  serialPrintln("");

  // Get the current PB values of the shift register (PB1-4)
  uint8_t PBvalues = pb1to4.getPBValues();

  static uint8_t PB1debounceCnt = 0;
  static uint8_t PB2debounceCnt = 0;
  static uint8_t PB3debounceCnt = 0;
  static uint8_t PB4debounceCnt = 0;

  // Check if buttons have been pressed and perform debounce
  if (PBvalues & (0x01) && !PB1debounceCnt) {  // PB1
    signalGenerator.previousSignalShape();
    PB1debounceCnt = 10;                                    // Start debouncer counter
  } else if (PBvalues & (0x01 << 1) && !PB2debounceCnt) {  // PB2
    signalGenerator.nextSignalShape();
    PB2debounceCnt = 10;
  }

  if (PBvalues & (0x01 << 2) && !PB3debounceCnt) {  // PB3
    signalGenerator.decreaseFrequency();
    PB3debounceCnt = 10;
  } else if (PBvalues & (0x01 << 3) && !PB4debounceCnt) {  // PB4
    signalGenerator.increaseFrequency();
    PB4debounceCnt = 10;
  }

  decreaseCounter(PB1debounceCnt);
  decreaseCounter(PB2debounceCnt);
  decreaseCounter(PB3debounceCnt);
  decreaseCounter(PB4debounceCnt);
  btnDecreaseAmplitude.evaluateDebounce();
  btnIncreaseAmplitude.evaluateDebounce();

  // Update PWM output duty-cycle
  DAC_IN.setDutyCycle(nextDatapoint);
}

/**
 * @brief callback function that is called when the decrease amplitude button (PB5) is pressed
 * It calls the decreaseAmplitude() function of the signalGenerator
 * @param buttonState state of the button (not used in this case)
 */
void decreaseAmplitudeCallback(ButtonState /*buttonState*/) {
  signalGenerator.decreaseAmplitude();
}

/**
 * @brief callback function that is called when the increase amplitude button (PB6) is pressed
 * It calls the increaseAmplitude() function of the signalGenerator
 * @param buttonState state of the button (not used in this case)
 */
void increaseAmplitudeCallback(ButtonState /*buttonState*/) {
  signalGenerator.increaseAmplitude();
}

int main() {
  initMSP();

  btnDecreaseAmplitude.init();
  btnDecreaseAmplitude.registerPressedStateChangeCallback(&decreaseAmplitudeCallback);

  btnIncreaseAmplitude.init();
  btnIncreaseAmplitude.registerPressedStateChangeCallback(&increaseAmplitudeCallback);

  pb1to4.init();

  DAC_IN.init();
  DAC_IN.setPwmPeriod<250, std::chrono::microseconds>();  // 4kHz

  Adc::getInstance().init();
  Adc::getInstance().startConversion();

  // Timer with CLK_DIV = 8 and since the period of SMCLK is 1us we also let the timer know that.
  constexpr TimerConfigBase<8, 1> TIMER_CONFIG(TimerClockSource::Option::SMCLK);
  Timer<1>::getTimer().init(TIMER_CONFIG);
  // Creates a 20ms periodic task
  TaskHandler<20, std::chrono::milliseconds> timerTask(&timerInterrupt, true);
  Timer<1>::getTimer().registerTask(TIMER_CONFIG, timerTask);

  while (true) {
    _no_operation();
  }

  return 0;
}

// Port 1 interrupt vector
#pragma vector = PORT1_VECTOR
__interrupt void Port_1_ISR(void) {
  // Get bit 5 and 6 => 0110 0000 = 0x60
  volatile const uint8_t pendingInterrupt = getRegisterBits(P1IFG, static_cast<uint8_t>(0x60), static_cast<uint8_t>(5));

  if (pendingInterrupt > 1) {
    btnIncreaseAmplitude.getDebouncer().pinStateChanged();
  } else {
    btnDecreaseAmplitude.getDebouncer().pinStateChanged();
  }

  const uint8_t clearFlag = (pendingInterrupt << 0x05);
  resetRegisterBits(P1IFG, clearFlag);  // clear interrupt flag
}
