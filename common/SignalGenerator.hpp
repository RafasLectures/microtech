#ifndef MICROTECH_SIGNALGENERATOR_HPP
#define MICROTECH_SIGNALGENERATOR_HPP

#include <cmath>
#include <cstdint>

namespace Microtech {

/**
 * @class SignalProperties
 * @brief Class for storing properties of the signal
 */
class SignalProperties {
public:
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
    currentFrequency = newFrequency;
    phaseStep = PI_TIMES_2*currentFrequency/samplingFreqHz;
  }

  /**
   * @brief increase the phase of the signal
   */
  void increasePhase() noexcept {
    // calculate new phase and keep it within 0 and 2pi.
    currentPhase = fmod(currentPhase + phaseStep, PI_TIMES_2);
  }

  /**
   * @brief get the current phase of the signal
   * @return current phase of the signal
   */
  float getCurrentPhase() const noexcept {
    return currentPhase;
  }

private:
  const uint16_t samplingFreqHz; ///< Sampling frequency of the signal
  uint16_t currentFrequency = 0; ///< Current frequency of the signal
  float phaseStep = 0;  ///< Current phase step of the signal
  float currentPhase = 0; ///< Current phase of the signal
  static constexpr float PI_TIMES_2 = 2*M_PI; ///< 2*PI constant
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
  virtual uint64_t getNextPoint(SignalProperties& signal) noexcept = 0;
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
  uint64_t getNextPoint(SignalProperties& signal) noexcept override {
    // value is between 0 and 1
    // sin gives value from -1 to 1, so we add 1 to get the output from 0 to 2
    const float value = sin(signal.getCurrentPhase()) + 1;

    // Multiply by 500 since our maximum value is 1000 and the maximum from the sine is 2.
    const uint64_t multiplication = value * 500;
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
  uint64_t getNextPoint(SignalProperties& signal) noexcept override{
    const float currentPhase = signal.getCurrentPhase();
    constexpr float DEG30_IN_RAD = M_PI/6;  // 30 degrees in radians
    constexpr float DEG60_IN_RAD = M_PI/3;  // 60 degrees in radians
    constexpr float DEG120_IN_RAD = 2*M_PI/3; // 120 degrees in radians
    constexpr float DEG150_IN_RAD = DEG30_IN_RAD + DEG120_IN_RAD; // 150 degrees in radians
    constexpr float DEG210_IN_RAD = DEG150_IN_RAD + DEG60_IN_RAD; // 210 degrees in radians
    constexpr float DEG330_IN_RAD = DEG210_IN_RAD + DEG120_IN_RAD; // 330 degrees in radians

    constexpr float slope = 1000/DEG60_IN_RAD;

    uint64_t retVal = 0;
    // check for the current phase
    if (currentPhase < DEG30_IN_RAD) {
      retVal = currentPhase*slope + 500;
    } else if(currentPhase > DEG30_IN_RAD && currentPhase < DEG150_IN_RAD) {
      retVal = 1000;
    } else if(currentPhase > DEG150_IN_RAD && currentPhase < DEG210_IN_RAD) {
      retVal = -currentPhase*slope + 3500;
    } else if(currentPhase > DEG210_IN_RAD && currentPhase < DEG330_IN_RAD) {
      retVal = 0;
    } else if(currentPhase > DEG330_IN_RAD) {
      retVal = currentPhase*slope - 5500;
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
  uint64_t getNextPoint(SignalProperties& signal) noexcept override{
    uint64_t retVal = 0;
    if (signal.getCurrentPhase() < M_PI) {
      retVal = 1000;
    } else {
      retVal = 0;
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
  uint64_t getNextDatapoint() noexcept{
    signalProperties.increasePhase();
    return activeSignal->getNextPoint(signalProperties);
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

private:
  Sinusoidal sinusoidal; ///< Sinusoidal signal object
  Trapezoidal trapezoidal; ///< Trapezoidal signal object
  Rectangular rectangular; ///< Rectangular signal object
  iSignalType* activeSignal; ///< pointer to the active signal
  SignalProperties signalProperties; ///< Signal Properties object
};


}  // namespace Microtech

#endif  // MICROTECH_SIGNALGENERATOR_HPP
