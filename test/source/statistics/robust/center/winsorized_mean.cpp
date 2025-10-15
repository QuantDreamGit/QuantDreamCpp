#include <iostream>
#include "quantdream/statistics/robust/center/winsorized_mean.h"


int main() {
  /** Example of usage of winsorized mean function
   * Similarly to trimmed mean, winsorized mean set the extreme values to the nearest
   * remaining values instead of removing them.
   * trim_fraction must be between 0 and 0.5
   */

  // -------------------------------------------------------
  // Parameters
  // -------------------------------------------------------
  double const trim_fraction = 0.1;

  // -------------------------------------------------------
  // Example 1: Compute Winsorized mean of a vector of integers
  // -------------------------------------------------------
  std::vector<int> std_vec({12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0});

  auto result = qd::statistics::robust::center::winsorized_mean<double>(std_vec, trim_fraction);

  std::cout << "std::vector \t| Winsorized mean of data is: " << result << std::endl;

  // -------------------------------------------------------
  // Example 2: Compute Winsorized mean of a Eigen::VectorXd
  // -------------------------------------------------------
  Eigen::VectorXd eigen_vec(13);
  eigen_vec << 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0;
  result = qd::statistics::robust::center::winsorized_mean<double>(eigen_vec, trim_fraction);

  std::cout << "Eigen::VectorXd | Winsorized mean of data is: " << result << std::endl;
}