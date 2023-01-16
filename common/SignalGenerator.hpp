#ifndef MICROTECH_SIGNALGENERATOR_HPP
#define MICROTECH_SIGNALGENERATOR_HPP

#include "IQmathLib.h"
#include <cstdint>

namespace Microtech {

#define PI      3.1415926536

/**
 * @class SignalProperties
 * @brief Class for storing properties of the signal
 */
class SignalProperties {
public:
  using PhaseType = _iq15;
  /**
   * @brief deleted default constructor
   */
  SignalProperties() = delete;
  /**
   * @brief constructor
   * @param[in] fsHz Sampling frequency of the signal
   *
   * This constructor also set the initial frequency of the signal to 1 Hz.
   */
  explicit SignalProperties(uint16_t fsHz) : samplingFreqHz(fsHz) {
    setNewFrequency(1);
  }

  /**
   * @brief set the new frequency of the signal
   * @param[in] newFrequency new frequency of the signal
   */
  void setNewFrequency(const uint16_t newFrequency) {
    currentFrequency = _IQ15(newFrequency);
    // 2*pi*currentFrequency/samplingFreqHz
    const _iq15 test = _IQ15mpy(_IQ15(2*PI),currentFrequency);
    phaseStep = _IQ15div(test, _IQ15(samplingFreqHz));
  }

  /**
   * @brief increase the phase of the signal
   */
  void increasePhase() noexcept {
    // calculate new phase and keep it within 0 and 2pi.
    currentPhase = (currentPhase + phaseStep)%_IQ15(2*PI);
  }

  /**
   * @brief get the current phase of the signal
   * @return current phase of the signal
   */
  PhaseType getCurrentPhase() const noexcept {
    return currentPhase;
  }

  uint16_t getCurrentFrequency() const noexcept {
      return _IQ15int(currentFrequency);
  }

private:
  const uint16_t samplingFreqHz; ///< Sampling frequency of the signal
  _iq15 currentFrequency = 0; ///< Current frequency of the signal
  PhaseType phaseStep = 0;  ///< Current phase step of the signal
  PhaseType currentPhase = 0; ///< Current phase of the signal
};


/**
 * @class iSignalType
 * @brief Interface for the different types of signals
 */
class iSignalType {
public:
  /**
   * @brief default constructor
   */
  iSignalType() = default;

  /**
   * @brief default destructor
   */
  virtual ~iSignalType() = default;

  /**
   * @brief pure virtual function to get the next point of the signal
   * @param[in] signal Signal properties
   * @return next point of the signal
   */
  virtual _iq15 getNextPoint(SignalProperties& signal) noexcept = 0;
};

/**
 * @brief Class representing a Sinusoidal signal
 */
class Sinusoidal : public iSignalType {
public:
  /**
   * @brief default constructor
   */
  Sinusoidal() = default;

  /**
   * @brief get the next point of the Sinusoidal signal
   * @param[in] signal Signal properties
   * @return next point of the Sinusoidal signal
   */
  _iq15 getNextPoint(SignalProperties& signal) noexcept override {
    // value is between 0 and 1
    // sin gives value from -1 to 1, so we add 1 to get the output from 0 to 2
    const _iq15 value = _IQ15sin(signal.getCurrentPhase()) + _IQ15(1);

    // Multiply by 50.0 since our maximum value is 100.0 and the maximum from the sine is 2.
    const _iq15 multiplication = _IQ15mpy(value, _IQ15(50.0));
    return multiplication;
  }
};

/**
 * @brief Class representing Trapezoidal signal
 */
class  Trapezoidal : public iSignalType {
public:
  /**
   * @brief default constructor
   */
  Trapezoidal() = default;

  /**
   * @brief get the next point of the Trapezoidal signal
   * @param[in] signal Signal properties
   * @return next point of the Trapezoidal signal
   */
  _iq15 getNextPoint(SignalProperties& signal) noexcept override{
    const _iq15 currentPhase = signal.getCurrentPhase();
    constexpr _iq15 DEG30_IN_RAD = _IQ15(PI/6);  // 30 degrees in radians
    constexpr _iq15 DEG60_IN_RAD = _IQ15(PI/3);  // 60 degrees in radians
    constexpr _iq15 DEG120_IN_RAD = _IQ15(2*PI/3); // 120 degrees in radians
    constexpr _iq15 DEG150_IN_RAD = DEG30_IN_RAD + DEG120_IN_RAD; // 150 degrees in radians
    constexpr _iq15 DEG210_IN_RAD = DEG150_IN_RAD + DEG60_IN_RAD; // 210 degrees in radians
    constexpr _iq15 DEG330_IN_RAD = DEG210_IN_RAD + DEG120_IN_RAD; // 330 degrees in radians

    const _iq15 slope = _IQ15div(_IQ15(100.0),DEG60_IN_RAD);

    _iq15 retVal = 0;
    // check for the current phase

    if (currentPhase < DEG30_IN_RAD) {
      retVal = _IQ15mpy(currentPhase,slope) + _IQ15(50.0);
    } else if(currentPhase > DEG30_IN_RAD && currentPhase < DEG150_IN_RAD) {
      retVal = _IQ15(100.0);
    } else if(currentPhase > DEG150_IN_RAD && currentPhase < DEG210_IN_RAD) {
      //retVal = -currentPhase*slope + 3500;
      retVal = _IQ15mpy(_IQ15(-1),_IQ15mpy(currentPhase,slope)) + _IQ15(350.0);
    } else if(currentPhase > DEG210_IN_RAD && currentPhase < DEG330_IN_RAD) {
      retVal = _IQ15(0);
    } else if(currentPhase > DEG330_IN_RAD) {
      //retVal = currentPhase*slope - 5500;
      retVal = _IQ15mpy(currentPhase,slope) - _IQ15(550.0);
    }
    return retVal;
  }
};

/**
 * @brief Class representing Rectangular signal
 */
class  Rectangular : public iSignalType {
public:
  /**
   * @brief default constructor
   */
  Rectangular() = default;

  /**
   * @brief get the next point of the Rectangular signal
   * @param[in] signal Signal properties
   * @return next point of the Rectangular signal
   */
  _iq15 getNextPoint(SignalProperties& signal) noexcept override{
    _iq15 retVal = 0;
    if (signal.getCurrentPhase() < _IQ15(PI)) {
      retVal = _IQ15(100.0);
    } else {
      retVal = _IQ15(0);
    }
    return retVal;
  }
};

/**
 * @brief Generates different types of signals (sinusoidal, trapezoidal, and rectangular) and switch between them.
 */
class SignalGenerator {
public:
  /**
   * @enum Shape
   * @brief Enumeration of different signal shapes that can be generated.
   */
  enum class Shape {
    SINUSOIDAL, ///< Signal shape representing sinusoidal signal
    TRAPEZOIDAL, ///< Signal shape representing trapezoidal signal
    RECTANGULAR, ///< Signal shape representing rectangular signal
  };

  /**
   * @brief Constructor is explicit and takes a single argument, a sampling frequency in Hz.
   * It sets the active signal to a sinusoidal signal and sets the signalProperties using the given sampling frequency.
   * @param[in] samplingFreqHz Sampling frequency in Hz
   */
  explicit SignalGenerator(uint16_t samplingFreqHz) : activeSignal(&sinusoidal), signalProperties(samplingFreqHz) {}

  /**
   * @brief get the next data point of the active signal
   * @return next data point
   */
  _iq15 getNextDatapoint() noexcept{
    signalProperties.increasePhase();
    return _IQ15mpy(activeSignal->getNextPoint(signalProperties), outputAmplitudePercentage) + _IQ15mpy(_IQ15(1.0) - outputAmplitudePercentage, _IQ15(100.0));
  }

  /**
   * @brief set a new frequency for the signal
   * @param[in] newFrequency new frequency
   */
  void setNewFrequency(const uint16_t newFrequency) noexcept{
    signalProperties.setNewFrequency(newFrequency);
  }

  /**
   * @brief set the active signal shape to one of the available types (sinusoidal, trapezoidal, or rectangular).
   * @param[in] newShape new shape of the signal
   */
  void setActiveSignalShape(const Shape newShape) noexcept {
    switch (newShape) {
      case Shape::SINUSOIDAL:
        activeSignal = &sinusoidal;
        break;
      case Shape::TRAPEZOIDAL:
        activeSignal = &trapezoidal;
        break;
      case Shape::RECTANGULAR:
        activeSignal = &rectangular;
        break;
    }
  }

  void nextSignalShape() {
      if(shapeIndex < 2) {
          shapeIndex++;
          setActiveSignalShape((Shape)shapeIndex);
      }
  }

  void previousSignalShape() {
    if(shapeIndex > 0) {
        shapeIndex--;
        setActiveSignalShape((Shape)shapeIndex);
    }
  }

  void increaseFrequency() {
      uint16_t currentFrequency = signalProperties.getCurrentFrequency();
      if(currentFrequency < 5) {
          signalProperties.setNewFrequency(currentFrequency + 1);
      }
  }

  void decreaseFrequency() {
      uint16_t currentFrequency = signalProperties.getCurrentFrequency();
      if(currentFrequency > 1) {
          signalProperties.setNewFrequency(currentFrequency - 1);
      }
  }

  void increaseAmplitude() {
      if(outputAmplitudePercentage < _IQ15(1.0)) {
          outputAmplitudePercentage += _IQ15(0.05);
      }
  }
  void decreaseAmplitude() {
        if(outputAmplitudePercentage > _IQ15(0.0)) {
            outputAmplitudePercentage -= _IQ15(0.05);
        }
    }
private:
  Sinusoidal sinusoidal; ///< Sinusoidal signal object
  Trapezoidal trapezoidal; ///< Trapezoidal signal object
  Rectangular rectangular; ///< Rectangular signal object
  iSignalType* activeSignal; ///< pointer to the active signal
  SignalProperties signalProperties; ///< Signal Properties object
  _iq15 outputAmplitudePercentage = _IQ15(1.0);
  uint8_t shapeIndex = 0;
};


}  // namespace Microtech

#endif  // MICROTECH_SIGNALGENERATOR_HPP
