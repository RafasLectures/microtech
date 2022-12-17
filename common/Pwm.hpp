/******************************************************************************
 * @file                    Pwm.hpp
 * @author                  Rafael Andrioli Bauer
 * @date                    14.12.2022
 * @matriculation number    5163344
 * @e-mail contact          abauer.rafael@gmail.com
 *
 * @brief   Header contains abstraction of PWM
 *
 * Description: In order to abstract the manipulation of registers and
 *              ease up maintainability of the application code, this header
 *              provides an easy interface to use the PWM
 ******************************************************************************/
#ifndef MICROTECH_PWM_HPP
#define MICROTECH_PWM_HPP

#include "Timer.hpp"

#include <msp430g2553.h>
#include <chrono>
#include <memory>

namespace Microtech {
/**
 * Class to abstract the PWM.
 */
class Pwm {
public:
  Pwm() = delete;
  explicit Pwm(const OutputHandle& outputPin) : pwmOutput(outputPin) {}

  void init() {
    Timer<0>::getTimer().init(TIMER_CONFIG);
    pwmOutput.init();
    setRegisterBits(pwmOutput.PxSel, pwmOutput.mBitMask);
    TA0CCTL2 = OUTMOD_3;
  }

  /**
   * Class to set the period of the PWM
   * At the moment we are using the timer 0 as the timer for the PWM.
   * So this method basically registers a task in the timer, and
   * since the init configured TA0CTTL2, the compar value of the timer will
   * be in the pwmOutput.
   */
  template<uint64_t periodValue, typename Duration = std::chrono::microseconds>
  void setPwmPeriod() {
    // Pointer of a newTask
    std::unique_ptr<TaskHandler<periodValue, Duration>> newTask =
      std::make_unique<TaskHandler<periodValue, Duration>>(nullptr, true);

    if (currentTask) {  // if there is a task playing, deregister the task
      Timer<0>::getTimer().deregisterTask(*currentTask);
    }
    // Register the newTask
    Timer<0>::getTimer().registerTask(TIMER_CONFIG, *newTask);

    // Update dutycycle, since comparator value changed.
    updateDutyCycleRegister();

    currentTask = std::move(newTask);  // Stores new task pointer.
  }

  /**
   * Method to set the PWM duty cycle.
   * The duty cycle can be between 0 and 1000. it is equivalent to 0 and 100.0
   */
  bool setDutyCycle(uint32_t newDutyCycle) {
    if (dutyCycle > MAX_DUTY_CYCLE) {
      return false;
    }
    dutyCycle = newDutyCycle;
    updateDutyCycleRegister();
    return true;
  }

  void stop() {
    Timer<0>::getTimer().stop();
  }

private:
  /**
   * Method to configure the register responsible by the duty cycle.
   */
  void updateDutyCycleRegister() {
    const uint32_t valueCCR0 = TACCR0;  // Reads current "period" register
    // const uint32_t halfDutyCycle = dutyCycle/2;
    //  Calculates the value of the CCR2 based on the value of the current CCR0 value.
    const uint32_t valueCCR2 = (valueCCR0 * dutyCycle /*+ halfDutyCycle*/) / MAX_DUTY_CYCLE;
    TA0CCR2 = valueCCR2;
  }

  static constexpr uint32_t MAX_DUTY_CYCLE = 1000;

  const OutputHandle pwmOutput;

  static constexpr TimerConfigBase<8> TIMER_CONFIG{};
  std::unique_ptr<TaskHandlerBase> currentTask;
  uint32_t dutyCycle = 0;
};

}  // namespace Microtech

#endif  // MICROTECH_PWM_HPP
