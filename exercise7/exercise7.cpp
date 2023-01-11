/*****************************************************************************
* @file                    exercise7.cpp
* @author                  Rafael Andrioli Bauer
* @date                    11.01.2023
* @matriculation number    5163344
* @e-mail contact          abauer.rafael@gmail.com
*
* @brief   Exercise 7 - Watchdog Timers & Timer Modules
*
* Description:
*
* Pin connections:
*       PB5 <-> CON3:P1.3
*       LED Green <-> CON3:P1.0
*       X3 <-> X10
*       JP4
*       X4 <-> CON3:P3.5
*       X5 <-> CON3:P3.4
*       LED Red <-> CON3:P3.2
*       U_NTC <-> CON3:P1.5
*
*
* Theory answers: None
*
* Tasks completed:
*  Task 1
*         [x] Cause deadlock after 5.5s              (x/2,0 pt.)
*  Task 2
*    a)
*         [x] Modify deadlock after 21.9s            (x/1,0 pt.)
*    b)
*         [x] Determine NTC range                    (x/1,0 pt.)
*    c)
*         [x] Thermometer with 0.5Hz refresh rate    (x/2,0 pt.)
*    d)
*         [x] Controller with X5                     (x/1,0 pt.)
*    e)
*         [x] Controller with X4                     (x/2,0 pt.)
*  Task 3
*         [x] feedback.txt                           (x/1.0 pt.)
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

//#define WATCHDOG_TIME_5_SECONDS
#define CONTROL_WITH_PWM
using namespace Microtech;

// Button infinite loop
Button PB5(GPIOs::getInputHandle<IOPort::PORT_1, static_cast<uint8_t>(3)>(), true);

// Temperature control part
#ifndef CONTROL_WITH_PWM
constexpr OutputHandle heatingResistorOnOffPin = GPIOs::getOutputHandle<IOPort::PORT_3, static_cast<uint8_t>(4)>();
#else
Pwm heatingResistorPwm(
  GPIOs::getOutputHandle<IOPort::PORT_3, static_cast<uint8_t>(5)>());  // Create handle of PWM for pin 6 from port 5
#endif
// NTC ADC value range: 320 - 570
AdcHandle NTC_input = Adc::getInstance().getAdcHandle<5>();

constexpr uint16_t NTC_MIN_VALUE = 320;
constexpr uint16_t NTC_MAX_VALUE = 570;
constexpr uint16_t NTC_INTERVAL_VALUE = (NTC_MAX_VALUE - NTC_MIN_VALUE)/5;

constexpr uint16_t VALUE_LED1 = NTC_MIN_VALUE + NTC_INTERVAL_VALUE;
constexpr uint16_t VALUE_LED2 = VALUE_LED1 + NTC_INTERVAL_VALUE;
constexpr uint16_t VALUE_LED3 = VALUE_LED2 + NTC_INTERVAL_VALUE;
constexpr uint16_t VALUE_LED4 = VALUE_LED3 + NTC_INTERVAL_VALUE;

ShiftRegisterLED ledD1ToD4(GPIOs::getOutputHandle<IOPort::PORT_2, static_cast<uint8_t>(4)>(),
                           GPIOs::getOutputHandle<IOPort::PORT_2, static_cast<uint8_t>(5)>(),
                           GPIOs::getOutputHandle<IOPort::PORT_2, static_cast<uint8_t>(0)>(),
                           GPIOs::getOutputHandle<IOPort::PORT_2, static_cast<uint8_t>(1)>(),
                           GPIOs::getOutputHandle<IOPort::PORT_2, static_cast<uint8_t>(6)>());
constexpr OutputHandle redLed = GPIOs::getOutputHandle<IOPort::PORT_3, static_cast<uint8_t>(2)>();

// Variable used to stay in infinite loop
bool enterInfiniteLoop = false;
uint8_t currentTemperatureRange = 1;
uint16_t currentNtcValue = 0;

void displaytemperatureTaskFunc() {
    static uint16_t numInterrupts = 0;
    if(numInterrupts < 2000) {
        numInterrupts++;
    } else {
        numInterrupts = 0;
        const uint16_t ntcValue = NTC_input.getRawValue();
        currentNtcValue = ntcValue;

        if(ntcValue < VALUE_LED1) {
            ledD1ToD4.writeValue(0x1);
            redLed.setState(IOState::LOW);
            currentTemperatureRange = 1;
        } else if(ntcValue < VALUE_LED2) {
            ledD1ToD4.writeValue(0x3);
            redLed.setState(IOState::LOW);
            currentTemperatureRange = 2;
        } else if(ntcValue < VALUE_LED3) {
            ledD1ToD4.writeValue(0x7);
            redLed.setState(IOState::LOW);
            currentTemperatureRange = 3;
        } else if(ntcValue < VALUE_LED4) {
            ledD1ToD4.writeValue(0xF);
            redLed.setState(IOState::LOW);
            currentTemperatureRange = 4;
        } else {
            ledD1ToD4.writeValue(0xF);
            redLed.setState(IOState::HIGH);
            currentTemperatureRange = 5;
        }
    }
}
void pb5Callback(ButtonState /*buttonState*/) {
   enterInfiniteLoop = true;
}

int main() {
 initMSP();

 // VLOCLK is 12 kHz according to datasheet, page 276.
 // Route VLOCLK to ACKL.
 BCSCTL3 |= LFXT1S_2;
#ifdef WATCHDOG_TIME_5_SECONDS
 BCSCTL1 |= DIVA_1;  // Set ACKL divider to 2.
 constexpr int64_t ACKL_CLK_PERIOD_US = 166;
#else
 BCSCTL1 |= DIVA_3;  // Set ACKL divider to 8.
 constexpr int64_t ACKL_CLK_PERIOD_US = 666;
#endif

 constexpr OutputHandle greenLed = GPIOs::getOutputHandle<IOPort::PORT_1, static_cast<uint8_t>(0)>();
 greenLed.init();
 greenLed.setState(IOState::HIGH);

 PB5.init();
 PB5.registerStateChangeCallback(&pb5Callback);


#ifndef CONTROL_WITH_PWM
 heatingResistorOnOffPin.init();
 heatingResistorOnOffPin.setState(IOState::LOW);
#else
 heatingResistorPwm.init();
#endif

 ledD1ToD4.init();
 ledD1ToD4.start();
 ledD1ToD4.writeValue(0x0);
 redLed.init();
 redLed.setState(IOState::LOW);


 Adc::getInstance().init();
 Adc::getInstance().startConversion();

 // Timer with CLK_DIV = 1 and since the period of ACLK is 83us we also let the timer know that.
 constexpr TimerConfigBase<1, 1> TIMER_CONFIG(TimerClockSource::Option::SMCLK);  // helper class where states whats the CLK_DIV of the timer.
 Timer<0>::getTimer().init(TIMER_CONFIG);
 // Creates a 2s periodic task to evaluate the thermometer
 TaskHandler<1, std::chrono::milliseconds> displayTemperatureTask(&displaytemperatureTaskFunc, true);
 Timer<0>::getTimer().registerTask(TIMER_CONFIG, displayTemperatureTask);

 // Enable watchdog
 WDTCTL = WDTPW + WDTCNTCL + WDTSSEL;

 // Initialize display
 displaytemperatureTaskFunc();

 while (true) {
   // Reset Watchdog
   WDTCTL = WDTPW + WDTCNTCL + WDTSSEL;
   while(enterInfiniteLoop) {}

   // Clock = 1 MHz. We want to have the LED blinking with 4Hz. It has a period of 0,25, but we need to divide it by
   // 2 because every half cycle we need to change output value. so 0,125 * 1000000 = 125000
   __delay_cycles(125000);
   greenLed.toggle();

#ifndef CONTROL_WITH_PWM
   if(currentTemperatureRange < 4) {
       heatingResistorOnOffPin.setState(IOState::HIGH);
   } else {
       heatingResistorOnOffPin.setState(IOState::LOW);
   }
#else
   if(currentTemperatureRange < 4) {
       heatingResistorPwm.setDutyCycle(500);
   } else {
       heatingResistorPwm.setDutyCycle(0);
   }
#endif
   serialPrint("Current NTC value: ");
   serialPrintInt(NTC_input.getRawValue());
   serialPrintln("");
 }

 return 0;
}

// Port 1 interrupt vector
#pragma vector = PORT1_VECTOR
__interrupt void Port_1_ISR(void) {
 // Get bit 3 => 0000 1000 = 0x08
 volatile const uint8_t pendingInterrupt = getRegisterBits(P1IFG, static_cast<uint8_t>(0x08), static_cast<uint8_t>(3));

 //if (pendingInterrupt == 1) {
    PB5.getDebouncer().pinStateChanged();
 //}

 const uint8_t clearFlag = (pendingInterrupt << 0x03);
 resetRegisterBits(P1IFG, clearFlag);  // clear interrupt flag
}

