//
// Created by Giuseppe Priolo on 08/09/25.
//

#ifndef BASIC_H
#define BASIC_H
#include <functional>
#include <limits>

namespace mafirm::math::derivative {
    double derivative(const std::function<double(double)>& f,
                      double x,
                      double h=std::sqrt(std::numeric_limits<double>::epsilon()));
}
#endif //BASIC_H
