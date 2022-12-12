#ifndef MICROTECH_PWM_HPP
#define MICROTECH_PWM_HPP

#include "Timer.hpp"

#include <chrono>
#include <memory>
#include <msp430g2553.h>

namespace Microtech {

class Pwm {
public:
  Pwm() = delete;
  explicit Pwm(const OutputHandle& outputPin) : pwmOutput(outputPin) {}

  void init() {
    pwmOutput.init();
    setRegisterBits(pwmOutput.PxSel, pwmOutput.mBitMask);
    TA0CCTL2 = OUTMOD_3;
  }

  template<uint64_t periodValue, typename Duration = std::chrono::microseconds>
  void setPwmPeriod() {
    constexpr TimerConfigBase<8> TIMER_CONFIG;
    std::unique_ptr<TaskHandler<periodValue, Duration>> newTask =
      std::make_unique<TaskHandler<periodValue, Duration>>(nullptr, true);

    if(currentTask) {
      Timer<0>::getTimer().deregisterTask(*currentTask);
    }
    // registers adc task to timer 0
    Timer<0>::getTimer().registerTask(TIMER_CONFIG, *newTask);

    updateDutyCycleRegister();

    currentTask = std::move(newTask);
  }

  bool setDutyCycle(uint32_t newDutyCycle) {
    if(dutyCycle > MAX_DUTY_CYCLE) {
      return false;
    }
    dutyCycle = newDutyCycle;
    updateDutyCycleRegister();
  }

private:
  void updateDutyCycleRegister() {
    const uint32_t valueCCR0 = TACCR0;
    //const uint32_t halfDutyCycle = dutyCycle/2;
    const uint32_t valueCCR2 = (valueCCR0*MAX_DUTY_CYCLE /*+ halfDutyCycle*/)/dutyCycle;
    TA0CCR2 = valueCCR2;
  }

  static constexpr uint32_t MAX_DUTY_CYCLE = 1000;

  const OutputHandle pwmOutput;

  std::unique_ptr<TaskHandlerBase> currentTask;
  uint32_t dutyCycle = 0;
};

}  // namespace Microtech

#endif  // MICROTECH_PWM_HPP
