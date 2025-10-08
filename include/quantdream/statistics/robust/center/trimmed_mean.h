//
// Created by user on 10/8/25.
//

#ifndef QUANTDREAMCPP_TRIMMED_MEAN_H
#define QUANTDREAMCPP_TRIMMED_MEAN_H

#include <Eigen/Dense>
#include <vector>

#include "quantdream/core/concepts/numeric_like.h"
#include "quantdream/core/concepts/numeric.h"
#include "quantdream/core/eigen_utils/stl_to_eigen.h"
#include "quantdream/core/algorithms/sort.h"

/**
 * Function template to compute the trimmed mean of a dataset.
 * Trimmed mean can be computed for any numeric type that can be stored in a std::vector.
 * The trimmed mean is calculated by removing a specified fraction of the smallest
 * and largest values from the dataset and then computing the mean of the remaining values.
 * T and RT can be any numeric type (e.g., int, float, double).
 *
 * @tparam Derived Type of elements in the input data (e.g., int, float, double).
 * @tparam RT Type to which elements will be cast during computation (default is double).
 * @param data A vector of double values representing the dataset.
 * @param trim_fraction A double value between 0 and 0.5 representing the fraction of data to trim from each end.
 *                      For example, a trim_fraction of 0.1 means that 10% of the smallest and 10% of the largest
 *                      values will be removed before calculating the mean.
 * @return The trimmed mean as a RT value (e.g., double).
 * @throws std::invalid_argument if trim_fraction is not in the range [0, 0.5] or if data is empty after trimming.
 */
template<typename RT = double, qd::concepts::numeric_like::NumericLike Derived>
RT trimmed_mean(Eigen::MatrixBase<Derived>& data, double const& trim_fraction) {
  // Check if trim_fraction is valid
  if (trim_fraction < 0.0 || trim_fraction > 0.5) {
    throw std::invalid_argument("trim_fraction must be between 0 and 0.5");
  }

  // Sort the data in place
  qd::algorithm::sort::sort_in_place(data, true);

  // Calculate the starting index for the mean
  int const data_size = static_cast<int>(data.size());
  auto k = static_cast<Eigen::Index>(std::floor(data_size * trim_fraction));

  // Compute the mean of the trimmed data
  RT trimmed_mean = data.segment(k, data_size - 2 * k).mean();

  return trimmed_mean;
}

/** Overload of trimmed_mean function to accept std::vector input.
 *
 * @tparam T Type of elements in the input std::vector (e.g., int, float, double).
 * @tparam RT Type to which elements will be cast during computation (default is double).
 * @param data A std::vector of values representing the dataset.
 * @param trim_fraction A double value between 0 and 0.5 representing the fraction of data to trim from each end.
 *                      For example, a trim_fraction of 0.1 means that 10% of the smallest and 10% of the largest
 *                      values will be removed before calculating the mean.
 * @return The trimmed mean as a RT value (e.g., double).
 * @throws std::invalid_argument if trim_fraction is not in the range [0, 0.5] or if data is empty after trimming.
 */
template<qd::concepts::math::Numeric RT = double, qd::concepts::math::Numeric T>
RT trimmed_mean(std::vector<T>& data, double const& trim_fraction) {
  // Create eigen vector from std::vector
  Eigen::VectorXd eigen_data = stl_to_eigen<T, RT>(data);

  return trimmed_mean<RT, Eigen::VectorXd>(eigen_data, trim_fraction);
}
#endif  // QUANTDREAMCPP_TRIMMED_MEAN_H
