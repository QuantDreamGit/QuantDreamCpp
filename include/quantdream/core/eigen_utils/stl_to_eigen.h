//
// Created by user on 10/8/25.
//

#ifndef QUANTDREAMCPP_STL_TO_EIGEN_H
#define QUANTDREAMCPP_STL_TO_EIGEN_H
#include <Eigen/Dense>
#include <vector>

#include "quantdream/core/concepts/numeric.h"
#include "quantdream/core/algorithms/sort.h"

/**
 * Function template to convert std::vector into Eigen::VectorXd.
 * Requires that the type T is double, float, or any integral type.
 * It is based on Eigen::Map functionality.
 * @requires std::floating_point<T> || std::integral<T>
 * @tparam T Type of elements in the std::vector (e.g., int, double).
 * @tparam RT Type to which elements will be cast during conversion (default is T).
 * @param vec Input std::vector to be converted.
 * @return Eigen::VectorXd containing the same elements as the input vector.
 * @throws std::invalid_argument if the input vector is empty.
 */
template<qd::concepts::math::Numeric T, qd::concepts::math::Numeric RT = T>
Eigen::VectorXd stl_to_eigen(std::vector<T> const& vec) {
  // Check if the input vector is empty
  if (vec.empty()) {
    throw std::invalid_argument("Input vector is empty");
  }

  // Create an Eigen::VectorXd of the same size as the input vector
  Eigen::VectorXd eigen_vec(vec.size());

  // Then iterate through the std::vector and copy each element to the Eigen::VectorXd
  // std::static_cast is used to convert each element to double as required by Eigen::VectorXd
  for (Eigen::Index i = 0; i < vec.size(); ++i) {
    eigen_vec(i) = static_cast<RT>(vec[i]);
  }

  return eigen_vec;
}


#endif  // QUANTDREAMCPP_STL_TO_EIGEN_H
