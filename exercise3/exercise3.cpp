/***************************************************************************//**
 * @file                    exercise3.cpp
 * @author                  Rafael Andrioli Bauer
 * @date                    18.11.2022
 * @matriculation number    5163344
 * @e-mail contact          abauer.rafael@gmail.com
 *
 * @brief   Exercise 3 - Interfacing External ICs
 *
 * Description:
 *
 * Pin connections:
 *
 * Theory answers: None.
 *
 * Tasks completed:
 *  Task 1
 *         [ ] Initial State                      (x/1,0 pt.)
 *         [ ] Playback PB3 pressed - 4 states/s  (x/1,0 pt.)
 *         [ ] Playback continues PB3 released    (x/1,0 pt.)
 *         [ ] Pause PB2                          (x/1,0 pt.)
 *         [ ] Fast-forward PB4 - 8 states/s      (x/1,0 pt.)
 *         [ ] Previous state PB4 released        (x/1,0 pt.)
 *         [ ] Rewind PB1 - 8 states/s            (x/1,0 pt.)
 *         [ ] Previous state PB1 released        (x/1,0 pt.)
 *         [ ] Respond to input regardless state  (x/1,0 pt.)
 *  Bonus
 *         [ ] D5 - D7 as rotating running light  (x/1.0 pt.)
 *
 *  Task 2
 *         [ ] feedback.txt                       (x/1.0 pt.)
 *
 * @note    The project was exported using CCS 12.1.0.00007
 ******************************************************************************/

#include "GPIOs.hpp"
#include "ShiftRegister.hpp"

using namespace Microtech;
int main() {
  OutputHandle s0Reg2 = GPIOs::getOutputHandle<IOPort::PORT_1>(0);

  s0Reg2.setState(IOState::HIGH);

  return 0;
}
