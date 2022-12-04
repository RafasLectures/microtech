#ifndef MICROTECH_ADC_HPP
#define MICROTECH_ADC_HPP

#include "helpers.hpp"

#include <msp430g2553.h>
#include <cstdint>
#include <vector>

namespace Microtech {

class AdcHandle {
  friend class Adc;
public:
  AdcHandle() = delete;
protected:
  constexpr AdcHandle(uint16_t& adcValuePtr) : rawValue(adcValuePtr) {}

  uint16_t& rawValue;
};

class Adc {
  Adc() = default;
public:
  ~Adc() = default;

  static Adc& getInstance() {
    static Adc instance;
    return instance;
  }
  void init() noexcept {
    // Repeat-sequence-of-channels mode
    // CLk source = ADC10OSC => around 5 MHz
    // CLK_DIV
    // Source of sample and hold from ADC10SC bit
    // Always start from A7
    ADC10CTL1 = CONSEQ_3 + ADC10SSEL0 + ADC10DIV0 + SHS0 + INCH_7;
    // ADC10 on
    // Multiple sample and conversion on.
    // sample and hold time = 16 ADC Clock cycles = 16*0.2us = 3.2 us
    ADC10CTL0 = ADC10ON + MSC + ADC10SHT_2;
    // need to set INCHx bits of ADC10CTL1 before starting.


    // Setup Data transfer control 0
    ADC10SA = adcValues[7];   //Starts at address;
    ADC10DTC0 = ADC10CT; // enable continuous transfer
    ADC10DTC1 = sizeof(adcValues)/sizeof(adcValues[0]);

  }

  void startConversion() {
    setRegisterBits(ADC10CTL0, static_cast<uint16_t>(ADC10SC + ENC));
  }

  template<uint8_t pinNumber>
  AdcHandle getAdcHandle() {
    setRegisterBits(ADC10AE0, pinNumber);
    return AdcHandle(adcValues[pinNumber]);
  }
  //ADC10MEM conversion is stored there
  // ADC10CTL0 and ADC10CTL1
  // enabled by ADC10ON bit
  // ADC10 control bits can only be modified when ENC = 0
  // ENC must be set to 1 before any conversion can take place.

private:
  uint16_t adcValues[8]{0,0,0,0,0,0,0,0};
};
}

// ADC10 Interruption
//#pragma vector = ADC10_VECTOR
//__interrupt void ADC10_ISR(void) {
//  Microtech::Adc::getInstance().interruptionHappened();
//}


#endif  // MICROTECH_ADC_HPP
