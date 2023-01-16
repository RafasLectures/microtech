/*****************************************************************************
* @file                    exercise8.cpp
* @author                  Rafael Andrioli Bauer
* @date                    11.01.2023
* @matriculation number    5163344
* @e-mail contact          abauer.rafael@gmail.com
*
* @brief   Exercise 9 - Project Option 1 - Oscilloscope and Signal generator
*
* Description:
*
* Pin connections:
*       CON3:P1.3 <-> DAC_OUT
*       JP5 -> INT
*       CON3:P3.6 <-> DAC_IN
*       CON3:P1.5 <-> PB5
*       CON3:P1.6 <-> PB6
*
* Theory answers: None
*
* Tasks completed:
*
*
* @note    The project was exported using CCS 12.1.0.00007
******************************************************************************/
#include <templateEMP.h>

#include "Button.hpp"
#include "GPIOs.hpp"
#include "Adc.hpp"
#include "ShiftRegister.hpp"
#include "Timer.hpp"
#include "Pwm.hpp"
#include "SignalGenerator.hpp"


using namespace Microtech;

// Buttons
Button btnDecreaseAmplitude(GPIOs::getInputHandle<IOPort::PORT_1, static_cast<uint8_t>(5)>(), true);
Button btnIncreaseAmplitude(GPIOs::getInputHandle<IOPort::PORT_1, static_cast<uint8_t>(6)>(), true);

// Oscilloscope channel
AdcHandle adcCH1 = Adc::getInstance().getAdcHandle<0>();

SignalGenerator signalGenerator(50);
Pwm DAC_IN(GPIOs::getOutputHandle<IOPort::PORT_3, static_cast<uint8_t>(6)>());  // Create handle of PWM for pin 6 from port 3

constexpr ShiftRegisterPB pb1to4(GPIOs::getOutputHandle<IOPort::PORT_2, static_cast<uint8_t>(4)>(),
                       GPIOs::getOutputHandle<IOPort::PORT_2, static_cast<uint8_t>(5)>(),
                       GPIOs::getOutputHandle<IOPort::PORT_2, static_cast<uint8_t>(2)>(),
                       GPIOs::getOutputHandle<IOPort::PORT_2, static_cast<uint8_t>(3)>(),
                       GPIOs::getInputHandle<IOPort::PORT_2, static_cast<uint8_t>(7)>());

uint16_t adcValue = 0;
uint16_t setpoint = 0;
bool newValue = false;

void decreaseCounter(uint8_t &counter) {
    if(counter > 0) {
        counter--;
    }
}

void timerInterrupt() {
    _iq15 nextDatapoint = signalGenerator.getNextDatapoint();
    // Print values of Oscilloscope
    adcValue = adcCH1.getRawValue();
    setpoint = _IQ15int(nextDatapoint);
    newValue = true;
    serialPrintInt(adcCH1.getRawValue());
   // serialPrint(" ");
    //serialPrintInt(_IQ15int(nextDatapoint));
    serialPrintln("");

    uint8_t PBvalues = pb1to4.getPBValues();
    static uint8_t PB1debounceCnt = 0;
    static uint8_t PB2debounceCnt = 0;
    static uint8_t PB3debounceCnt = 0;
    static uint8_t PB4debounceCnt = 0;

    if(PBvalues & (0x01) && !PB1debounceCnt) { //PB1
        signalGenerator.nextSignalShape();
        PB1debounceCnt = 5;
    } else if(PBvalues & (0x01 << 1) && !PB2debounceCnt) {  // PB2
        signalGenerator.previousSignalShape();
        PB2debounceCnt = 5;
    }

    decreaseCounter(PB1debounceCnt);
    decreaseCounter(PB2debounceCnt);

    if(PBvalues & (0x01 << 2) && !PB3debounceCnt) { //PB3
        signalGenerator.increaseFrequency();
        PB3debounceCnt = 5;
    } else if(PBvalues & (0x01 << 3) && !PB4debounceCnt) {  // PB4
        signalGenerator.decreaseFrequency();
        PB4debounceCnt = 5;
    }
    decreaseCounter(PB3debounceCnt);
    decreaseCounter(PB4debounceCnt);

    btnDecreaseAmplitude.evaluateDebounce();
    btnIncreaseAmplitude.evaluateDebounce();

    DAC_IN.setDutyCycle(nextDatapoint);
}

void decreaseAmplitudeCallback(ButtonState /*buttonState*/) {
    signalGenerator.decreaseAmplitude();
}

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
 DAC_IN.setPwmPeriod<250, std::chrono::microseconds>(); //4kHz


 Adc::getInstance().init();
 Adc::getInstance().startConversion();

 // Timer with CLK_DIV = 8 and since the period of SMCLK is 1us we also let the timer know that.
 constexpr TimerConfigBase<8, 1> TIMER_CONFIG(TimerClockSource::Option::SMCLK);
 Timer<1>::getTimer().init(TIMER_CONFIG);
 // Creates a 10ms periodic task
 TaskHandler<20, std::chrono::milliseconds> timerTask(&timerInterrupt, true);
 Timer<1>::getTimer().registerTask(TIMER_CONFIG, timerTask);

 //signalGenerator.setActiveSignalShape(SignalGenerator::Shape::TRAPEZOIDAL);
 while (true) {
     /*
     uint8_t PBvalues = pb1to4.getPBValues();

     if(PBvalues & (0x01)) { //PB1
         signalGenerator.nextSignalShape();
     } else if(PBvalues & (0x01 << 1)) {  // PB2
         signalGenerator.previousSignalShape();
     }

     if(PBvalues & (0x01 << 2)) { //PB3
         signalGenerator.increaseFrequency();
     } else if(PBvalues & (0x01 << 3)) {  // PB4
         signalGenerator.decreaseFrequency();
     }
     if(newValue) {
         serialPrintInt(adcValue);
         //serialPrint(" ");
         //serialPrintInt(setpoint);
         serialPrintln("");
         newValue = false;
     }
     */
 }

 return 0;
}

// Port 1 interrupt vector
#pragma vector = PORT1_VECTOR
__interrupt void Port_1_ISR(void) {
 // Get bit 5 and 6 => 0110 0000 = 0x60
 volatile const uint8_t pendingInterrupt = getRegisterBits(P1IFG, static_cast<uint8_t>(0x60), static_cast<uint8_t>(5));

 if(pendingInterrupt > 1) {
     btnIncreaseAmplitude.getDebouncer().pinStateChanged();
 } else {
     btnDecreaseAmplitude.getDebouncer().pinStateChanged();
 }
 //

 const uint8_t clearFlag = (pendingInterrupt << 0x05);
 resetRegisterBits(P1IFG, clearFlag);  // clear interrupt flag
}

