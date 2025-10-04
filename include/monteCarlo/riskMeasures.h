//
// Created by user on 9/26/25.
//

#ifndef QUANTDREAMCPP_RISKMEASURES_H
#define QUANTDREAMCPP_RISKMEASURES_H
#include <vector>
#include <Eigen/Dense>

// Generic function will be used to avoid code duplication for the different risk measures
// Define possible risk measures as enum
enum class RiskMeasure {
  VaR,
  ES
};

// Function to compute the portfolio risk measure (VaR or ES) for each simulation
// using the simulated returns and the weights of the assets in the portfolio
std::vector<double> computePortfolioRiskMeasures(const std::vector<Eigen::MatrixXd> &simulatedReturns,
                                             const std::vector<double> &weights,
                                             const size_t &alpha,
                                             const RiskMeasure &measure,
                                             bool plotLosses = false);

// Function to compute the Value at Risk (VaR)  and Expected Shortfall (ES)
// at a given confidence level alpha
/*
std::vector<double> computeVaR(const Eigen::MatrixXd &simulatedReturns,
                               const std::vector<double> &weights,
                               const size_t &alpha);

std::vector<double> computeES(const Eigen::MatrixXd &simulatedReturns,
                              const std::vector<double> &weights,
                              const size_t &alpha);
*/

#endif  // QUANTDREAMCPP_RISKMEASURES_H
