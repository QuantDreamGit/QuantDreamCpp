//
// Created by user on 10/8/25.
//

#ifndef QUANTDREAMCPP_EIGEN_H
#define QUANTDREAMCPP_EIGEN_H

#include <Eigen/Dense>

namespace qd::concepts::eigen {

/**
 * Concept to check if a type is an Eigen vector (either row or column vector).
 * This concept can be used to constrain template parameters to ensure
 * that only Eigen vector types are accepted.
 * @tparam Derived Type to be checked.
 */
template<typename Derived>
concept EigenVector =
  std::is_base_of_v<Eigen::DenseBase<Derived>, Derived> &&              // Check if Derived is derived from Eigen::DenseBase
  (Derived::ColsAtCompileTime == 1 || Derived::RowsAtCompileTime == 1); // Ensure it's a vector (either single column or single row)

}

#endif  // QUANTDREAMCPP_EIGEN_H
