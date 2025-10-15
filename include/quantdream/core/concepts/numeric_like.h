//
// Created by user on 10/8/25.
//

#ifndef QUANTDREAMCPP_NUMERIC_LIKE_H
#define QUANTDREAMCPP_NUMERIC_LIKE_H

#include "quantdream/core/concepts/numeric.h"
#include "quantdream/core/concepts/eigen.h"

namespace qd::concepts::numeric_like {
  /**
   * Concept to check if a type is either a numeric type or an Eigen vector.
   * This concept can be used to constrain template parameters to ensure
   * that only numeric types or Eigen vector types are accepted.
   * @tparam T Type to be checked.
   */
  template<typename T>
  concept NumericLike = qd::concepts::math::Numeric<T> || qd::concepts::eigen::EigenVector<T>;
}
#endif  // QUANTDREAMCPP_NUMERIC_LIKE_H
