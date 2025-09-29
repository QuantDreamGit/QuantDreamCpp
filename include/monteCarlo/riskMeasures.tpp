#include <eigen3/Eigen/Dense>
#include "monteCarlo/riskMeasures.h"

// Specializations for Eigen::VectorXd
// This is the most basic case, we just have to compute the risk measure on the vector of returns
// of a single asset.
template<typename T>
std::vector<double> computeRisk<Eigen::VectorXd> (
  const Eigen::VectorXd &returns,
  const RiskMeasure measure,
  const double &alpha) {

  // Compute final return
  double finalReturn = 1.0;
  for (size_t i = 0; i < returns.size(); i++) {
    finalReturn *= (1.0 + returns[i]);
  }
  finalReturn -= 1.0;

  if (measure == RiskMeasure::VaR) {
    return {alpha};
  } else if (measure == RiskMeasure::ES) {
    return {alpha};
  } else {
    throw std::runtime_error("Unknown risk measure!");
  }
}