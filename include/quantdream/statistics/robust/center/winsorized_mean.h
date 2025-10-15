//
// Created by user on 10/8/25.
//

#ifndef QUANTDREAMCPP_WINSORIZED_MEAN_H
#define QUANTDREAMCPP_WINSORIZED_MEAN_H

#include <Eigen/Dense>
#include <vector>

#include "quantdream/core/concepts/numeric.h"
#include "quantdream/core/concepts/numeric_like.h"
#include "quantdream/core/eigen_utils/stl_to_eigen.h"
#include "quantdream/core/algorithms/sort.h"

namespace qd::statistics::robust::center {
  /**
   * Function template to compute the winsorized mean of a dataset.
   * Winsorized mean can be computed for any numeric type that can be stored in a std::vector.
   * The winsorized mean is calculated by replacing a specified fraction of the smallest
   * and largest values from the dataset with the nearest remaining values and then computing the mean of the modified dataset.
   * T and RT can be any numeric type (e.g., int, float, double).
   *
   * @tparam Derived Type of elements in the input data (e.g., int, float, double).
   * @tparam RT Type to which elements will be cast during computation (default is double).
   * @param data A vector of double values representing the dataset.
   * @param trim_fraction A double value between 0 and 0.5 representing the fraction of data to winsorize from each end.
   *                      For example, a trim_fraction of 0.1 means that 10% of the smallest and 10% of the largest
   *                      values will be replaced before calculating the mean.
   * @return The winsorized mean as a RT value (e.g., double).
   * @throws std::invalid_argument if trim_fraction is not in the range [0, 0.5] or if data is empty after winsorization.
   */
  template<typename RT = double, qd::concepts::numeric_like::NumericLike Derived>
  RT winsorized_mean(Eigen::MatrixBase<Derived> const& data, double const& trim_fraction) {
    // Copy data to a mutable Eigen vector
    Eigen::VectorXd data_copy = data.template cast<RT>();

    // Check if trim_fraction is valid
    if (trim_fraction < 0.0 || trim_fraction > 0.5) {
      throw std::invalid_argument("trim_fraction must be between 0 and 0.5");
    }

    // Sort the data in place
    qd::algorithm::sort::sort_in_place(data_copy, true);

    // Calculate the starting index for the mean
    int const data_size = static_cast<int>(data_copy.size());
    auto k = static_cast<Eigen::Index>(std::floor(data_size * trim_fraction));

    // Set head and tail values to nearest remaining values
    data_copy.head(k).setConstant(data_copy(k));
    data_copy.tail(k).setConstant(data_copy(data_size - k - 1));

    // Compute the mean of the winsorized data
    RT winsorized_mean = data_copy.mean();

    return winsorized_mean;
  }

  /** Overload of winsorized_mean function to accept std::vector input.
   *
   * @tparam T Type of elements in the input std::vector (e.g., int, float, double).
   * @tparam RT Type to which elements will be cast during computation (default is double).
   * @param data A std::vector of values representing the dataset.
   * @param trim_fraction A double value between 0 and 0.5 representing the fraction of data to winsorize from each end.
   *                      For example, a trim_fraction of 0.1 means that 10% of the smallest and 10% of the largest
   *                      values will be replaced before calculating the mean.
   * @return The winsorized mean as a RT value (e.g., double).
   * @throws std::invalid_argument if trim_fraction is not in the range [0, 0.5] or if data is empty after winsorization.
   */
  template<qd::concepts::math::Numeric RT = double, qd::concepts::math::Numeric T>
  RT winsorized_mean(std::vector<T>& data, double const& trim_fraction) {
    // Create eigen vector from std::vector
    Eigen::VectorXd eigen_data = stl_to_eigen<T, RT>(data);

    return winsorized_mean<RT, Eigen::VectorXd>(eigen_data, trim_fraction);
  }
}
#endif  // QUANTDREAMCPP_WINSORIZED_MEAN_H
