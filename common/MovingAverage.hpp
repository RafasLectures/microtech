#ifndef MICROTECH_MOVINGAVERAGE_HPP
#define MICROTECH_MOVINGAVERAGE_HPP

#include <cstdint>

namespace Microtech {

/**
 * Class implements a simple moving average. of the equation:
 * N = number of samples
 * average[n+1] = average[n] + input[n+1] - input[n+1-N]
 */
template<uint8_t numSamples, class INPUT_TYPE = uint16_t, class SUM_TYPE = uint32_t>
class SimpleMovingAverage {
    static_assert(SUM_TYPE(0) < SUM_TYPE(-1), "Error: sum data type should be unsigned."); // Check that `sum_t` is an unsigned type
  public:
    INPUT_TYPE operator()(INPUT_TYPE input) {
        // Subtracts the value to be removed from the circular buffer
        sum -= previousInputs[index];
        // Adds the new input to the sum
        sum += input;
        previousInputs[index] = input;

        // Set index to 0 if it is the last value of the array to guarantee
        // circular buffer
        if (++index == numSamples) {
            index = 0;
        }

        // This is used to round the average value.
        // When evaluating the equation, this term adds a 1/2 to the average.
        // Since we are working with integers, it guarantees that if sum/numSamples is rounded.
        // For example, if the result of sum/numSamples is 234.2,
        // when added 0,5, it will be rounded to 234,
        // but if it is 234.6, when adding 0,5 it is 235.1 and it will be 235.
        constexpr SUM_TYPE halfNumSamples = (numSamples / 2);

        SUM_TYPE retVal = (sum + halfNumSamples) / numSamples;
        return static_cast<INPUT_TYPE>(retVal);
    }

  private:
    uint8_t index = 0;
    INPUT_TYPE previousInputs[numSamples] = {};
    SUM_TYPE sum = 0;
};
}
#endif
