//
// Created by user on 10/8/25.
//

#ifndef QUANTDREAMCPP_NUMERIC_H
#define QUANTDREAMCPP_NUMERIC_H

#include <concepts>

namespace qd::concepts::math {
    /**
     * Concept to check if a type is numeric (integral or floating point).
     * This concept can be used to constrain template parameters to ensure
     * that only numeric types are accepted.
     * Admitted types include int, float, double, long, etc.
     * @tparam T Type to be checked.
     */
    template<typename T>
    concept Numeric = std::integral<T> || std::floating_point<T>;
}

#endif  // QUANTDREAMCPP_NUMERIC_H
