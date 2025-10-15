//
// Created by user on 10/8/25.
//

#include <iostream>
#include "quantdream/statistics/robust/center/trimmed_mean.h"

int main() {
  /** Example usage of trimmed_mean function
   * The trimmed_mean function computes the mean of a dataset after removing a specified fraction
   * trim_fraction must be between 0 and 0.5
   */

  // -------------------------------------------------------
  // Parameters
  // -------------------------------------------------------
  double trim_fraction = 0.1;

  // -------------------------------------------------------
  // Example 1: Compute trimmed mean of a vector of integers
  // -------------------------------------------------------
  std::vector<int> std_vec({12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0});

  auto result = qd::statistics::robust::center::trimmed_mean<double>(std_vec, trim_fraction);

  std::cout << "std::vector \t| Trimmed mean of data is: " << result << std::endl;

  // -------------------------------------------------------
  // Example 2: Compute trimmed mean of a Eigen::VectorXd
  // -------------------------------------------------------
  Eigen::VectorXd eigen_vec(13);
  eigen_vec << 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0;
  result = qd::statistics::robust::center::trimmed_mean<double>(eigen_vec, trim_fraction);

  std::cout << "Eigen::VectorXd | Trimmed mean of data is: " << result << std::endl;
}