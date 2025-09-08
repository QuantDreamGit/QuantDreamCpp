// amir.khorrami@carloalberto.org
#include <iostream>
#include <__ostream/basic_ostream.h>
#include <autodiff/forward/dual/dual.hpp>

#include "autodiff/forward/utils/derivative.hpp"
#include "mafirm/math/basic.h"

using namespace autodiff;

dual f(dual x) { return x * x + 3; }

int main () {
  // Function to differentiate
  // std::function<double(double)> f = [](double x) { return x * x; };
  // double result = mafirm::math::derivative::derivative(f, 5);
  dual x = 1.0;
  dual1st y = 1.0;


  auto [u, ux] = derivatives(f, wrt(y), at(x));

  std::cout << u << " " << ux << std::endl;
  return 1;
}