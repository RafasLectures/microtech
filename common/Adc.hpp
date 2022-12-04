#ifndef MICROTECH_ADC_HPP
#define MICROTECH_ADC_HPP

#include "helpers.hpp"

#include <msp430g2553.h>
#include <cstdint>
#include <map>

namespace Microtech {

class AdcHandle {
  friend class Adc;
public:
  AdcHandle() = delete;
  uint16_t getRawValue() {
      return rawValue;
  }
protected:
  constexpr AdcHandle(uint16_t& adcValuePtr) : rawValue(adcValuePtr) {}

  void setRawValReference(uint16_t& adcValueRef) {
      rawValue = adcValueRef;
  }
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

    // ADC10 on
    // Multiple sample and conversion on.
    // sample and hold time = 16 ADC Clock cycles = 16*0.2us = 3.2 us
    ADC10CTL0 = ADC10ON + ADC10SHT_2 + MSC; //+ ADC10IE;

    // Repeat-sequence-of-channels mode
    // CLk source = ADC10OSC => around 5 MHz
    // CLK_DIV
    // Source of sample and hold from ADC10SC bit
    // Always start from A7
    ADC10CTL1 = CONSEQ_3 + ADC10SSEL_0 + ADC10DIV_0 + SHS_0 + INCHVal;

    // Setup Data transfer control 0
    ADC10DTC0 = ADC10CT; // enable continuous transfer
    ADC10DTC1 = 8;//sizeof(adcValues)/sizeof(adcValues[0]);
    ADC10SA = (uint16_t)&(adcValues[0]);   //Starts at address;
    // need to set INCHx bits of ADC10CTL1 before starting.




  }

  void startConversion() {
    setRegisterBits(ADC10CTL0, static_cast<uint16_t>(ADC10SC + ENC));
  }

  template<uint8_t pinNumber, uint8_t bitMask = 0x01 << pinNumber, uint16_t CTL1Bits = pinNumber*0x1000>
  AdcHandle getAdcHandle() {
      static_assert(pinNumber <= 7, "Cannot set ADC to pin higher than 7");
    setRegisterBits(ADC10AE0, bitMask);
    if(INCHVal < CTL1Bits) {
        INCHVal = CTL1Bits;
    }

    AdcHandle retVal(adcValues[7-pinNumber]);

    return std::move(retVal);
  }
  //ADC10MEM conversion is stored there
  // ADC10CTL0 and ADC10CTL1
  // enabled by ADC10ON bit
  // ADC10 control bits can only be modified when ENC = 0
  // ENC must be set to 1 before any conversion can take place.

private:
  uint16_t INCHVal = 0;
  uint16_t adcValues[8]{0,0,0,0,0,0,0,0};
  //std::map<uint8_t, AdcHandle*> handles;
};
}

// ADC10 Interruption
//#pragma vector = ADC10_VECTOR
//__interrupt void ADC10_ISR(void) {
//  Microtech::Adc::getInstance().interruptionHappened();
//}


#endif  // MICROTECH_ADC_HPP
