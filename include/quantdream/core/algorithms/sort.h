//
// Created by user on 10/8/25.
//

#ifndef QUANTDREAMCPP_SORT_H
#define QUANTDREAMCPP_SORT_H

#include <algorithm>
#include <functional>

#include "quantdream/core/concepts/eigen.h"

namespace qd::algorithm::sort {

/**
 * Function template to sort a std::vector or an Eigen vector in place.
 * The function can sort in ascending or descending order based on the 'ascending' parameter.
 *
 * @tparam T Type of elements in the std::vector (e.g., int, double) or Eigen vector.
 * @param data A reference to the std::vector or Eigen vector to be sorted.
 * @param ascending A boolean flag indicating whether to sort in ascending order (default is true).
 *                  If false, the function sorts in descending order.
 */
template<typename T>
void sort_in_place(std::vector<T>& data, const bool ascending = true) {
  if (ascending) {
    std::sort(data.begin(), data.end());
  } else {
    std::sort(data.begin(), data.end(), std::greater<T>());
  }
}

template<qd::concepts::eigen::EigenVector Derived>
void sort_in_place(Eigen::MatrixBase<Derived>& data, const bool ascending = true) {
  if (ascending) {
    std::sort(data.derived().data(), data.derived().data() + data.size());
  } else {
    // Specify std::greater to avoid ambiguity, use typename Derived::Scalar to ensure correct type
    std::sort(data.derived().data(), data.derived().data() + data.size(), std::greater<typename Derived::Scalar>());
  }
}



}

#endif  // QUANTDREAMCPP_SORT_H
