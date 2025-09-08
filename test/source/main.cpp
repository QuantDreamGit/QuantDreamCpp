// amir.khorrami@carloalberto.org
#include <iostream>
#include <__ostream/basic_ostream.h>

#include "mafirm/math/basic.h"

int main () {
    // Function to differentiate
    std::function<double(double)> f = [](double x) { return x * x; };
    double result = mafirm::math::derivative::derivative(f, 0);
    std::cout << result << std::endl;
    return 1;
}