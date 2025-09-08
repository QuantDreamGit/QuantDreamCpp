//
// Created by Giuseppe Priolo on 08/09/25.
//
#include <functional>
#include <limits>
#include "mafirm/math/basic.h"


namespace mafirm::math::derivative {
    double derivative(const std::function<double(double)>& f,
                      double x,
                      double h) {
        /**
         * @brief Computes the derivative of a function at point x using the central difference method.
         * @param f The function for which to compute the derivative.
         * @param x The point at which to compute the derivative.
         * @param h A small step size.
         * @return The approximate derivative of the function at point x.
         * @note The error is o(h^2), meaning it decreases quadratically as h approaches zero.
         * @note Default value for h try to balance accuracy and numerical stability.
         */
        // Offset h to improve numerical stability
        h = h * (1 + std::abs(x));
        return (f(x + h) - f(x - h)) / (2 * h);
    }
}
