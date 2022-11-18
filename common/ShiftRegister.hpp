/******************************************************************************
 * @file                    ShiftRegister.hpp
 * @author                  Rafael Andrioli Bauer
 * @date                    18.11.2022
 * @matriculation number    5163344
 * @e-mail contact          abauer.rafael@gmail.com
 *
 * @brief   Header contains abstraction of a shift register
 *
 * Description: In order to abstract the manipulation of the shift register and
 *              ease up maintainability of the application code, this header
 *              provides an easy interface to manipulate the shift register
 *
 *              The interface was developed using template classes as well as
 *              constexpr so the program memory usage would be optimized.
 ******************************************************************************/

#ifndef COMMON_SHIFTREGISTER_HPP_
#define COMMON_SHIFTREGISTER_HPP_

#include "GPIOs.hpp"
namespace Microtech {

class ShiftRegisterInterpreter {
public:
  enum class Mode {
    PAUSE = 0,        ///< The shift register stops to change the output regardless of the clock
    SHIFT_RIGHT,      ///< Shifts the every output to the right. The value of QA is defined the serial input right (SR)
    SHIFT_LEFT,       ///< Shifts the every output to the left. The value of QD is defined the serial input left (SL)
    MIRROR_PARALLEL,  ///< The outputs QA~D will output the steady state input A~D
  };

  ShiftRegisterInterpreter() = default;

  void setMode(Mode mode);

  //void setS0Pin(GPIOs<ioPort>::OutputHandle<>);

private:
  OutputHandle* s0;
};

class ShiftRegisterManager {
public:

};
} /* namespace Microtech */

#endif /* COMMON_SHIFTREGISTER_HPP_ */
