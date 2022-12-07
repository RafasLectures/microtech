/*****************************************************************************
 * @file                    exercise5.cpp
 * @author                  Rafael Andrioli Bauer
 * @date                    07.12.2022
 * @matriculation number    5163344
 * @e-mail contact          abauer.rafael@gmail.com
 *
 * @brief   Exercise 5 - Pulse Width Modulation
 *
 * Description:
 *
 * Pin connections:  BUZZER <-> CON3:P3.6
 *                   PB5 <-> CON3:P1.3
 *                   PB6 <-> CON3:P1.4
 *                   JP5 to VFO
 *
 * Theory answers: None.
 *
 * Tasks completed:
 *  Task 1
 *    a)   [ ] PWM with 50% duty-cycle            (x/1,0 pt.)
 *         [ ] Store melody in array              (x/1,0 pt.)
 *         [ ] Melody 1                           (x/1,0 pt.)
 *         [ ] Melody 2                           (x/1,0 pt.)
 *   b)
 *         [ ] Capture PB5 with interrupt         (x/1,0 pt.)
 *         [ ] Press once play melody one         (x/1,0 pt.)
 *         [ ] Press twice play melody two        (x/1,0 pt.)
 *   c)
 *         [ ] Detect board knock from piezo P3IN (x/2,0 pt.)
 *         [ ] Press twice play melody two        (x/1,0 pt.)
 *   d)
 *         [ ] PB6 as a pause/resume button       (x/1,0 pt.)
 *
 *  Task 2
 *         [ ] feedback.txt                       (x/1.0 pt.)
 *
 * @note    The project was exported using CCS 12.1.0.00007
 ******************************************************************************/
#include <templateEMP.h>

#include "GPIOs.hpp"
#include "Timer.hpp"

#include "Adc.hpp"

#include <chrono>

using namespace Microtech;

void evaluateAdcTask() {

}

using Timer0 = Timer<0, 8>;

int main() {
  initMSP();

  Timer0::getTimer().init();


  // Creates a 10ms periodic task for evaluating the adc values.
  TaskHandler<10, std::chrono::milliseconds> adcTask(&evaluateAdcTask, true);

  // registers adc task to timer 0
  Timer0::getTimer().registerTask(adcTask);

  // globally enables the interrupts.
  __enable_interrupt();
  while (true) {
    _no_operation();
  }
  return 0;
}
