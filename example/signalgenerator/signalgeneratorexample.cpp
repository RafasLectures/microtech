
#include <cstdint>
#include <vector>
#include "SignalGenerator.hpp"

#include "matplotlibcpp.h"

namespace plt = matplotlibcpp;
using namespace Microtech;
int main() {
  SignalGenerator signalGenerator(1000);


  // Prepare data.
  int n = 5000;
  std::vector<double> x(n), y(n);
  bool freqUpdated = false;
  for(int i=0; i<n; ++i) {
    if(i == 2250 && !freqUpdated) {
      freqUpdated = true;
      signalGenerator.setNewFrequency(2);
      signalGenerator.setActiveSignalShape(SignalGenerator::Shape::TRAPEZOIDAL);
      //activeSignal = &rectangular;
    }
    x.at(i) = i;
    y.at(i) = signalGenerator.getNextDatapoint();
  }

  // Set the size of output image to 1200x780 pixels
  //plt::figure_size(1200, 780);
  // Plot line from given x and y data. Color is selected automatically.
  plt::plot(x, y);
  // Set x-axis to interval [0,1000000]
  //plt::xlim(0, 5000);
  plt::ylim(0, 1000);
  // Add graph title
  plt::title("Sample figure");
  // Enable legend.
  plt::legend();
  // show plots
  plt::show();
  return 0;
}
