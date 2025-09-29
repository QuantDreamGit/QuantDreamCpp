//
// Created by Giuseppe Priolo on 29/09/25.
//
#include <Eigen/Dense>
#include <vector>
#include <algorithm>
#include <stdexcept>
#include "monteCarlo/riskMeasures.h"

// Matrix structure:
// Cols: Loss_asset_0 | ... | Loss_asset_N-1 | Portfolio_Loss
// Rows: Simulation_0
//        .
//        .
//        .
// Rows: Simulation_M-1
Eigen::MatrixXd computePortfolioRiskMeasures(const std::vector<Eigen::MatrixXd> &simulatedReturns,
                                             const std::vector<double> &weights,
                                             const size_t &alpha,
                                             const RiskMeasure &measure) {
  // Get the dimension of the output matrix
  const size_t nSimulations = simulatedReturns.size();
  const size_t nAssets = weights.size();

  // Construct the random variable
  Eigen::MatrixXd riskMeasures(nSimulations, nAssets + 1);

  // Select the correct function to compute the risk measure
  // auto riskFunction = (measure == RiskMeasure::VaR) ? computeVaR : computeES;

  // For each simulation, compute the portfolio loss and the losses of each asset
  for (size_t i = 0; i < nSimulations; i++) {
    // Compute the portfolio losses of the i-th simulation
    // To compute the loss, we can use the formula:
    // loss_j = prod (1 + return_ij)
    // total_loss_j = 1 - prod (1 + loss_j)
    // Transposition is needed to have a column vector of losses
    Eigen::VectorXd riskContributions = ((simulatedReturns[i].array() + 1).colwise().prod() - 1).transpose();

    // Convert weights to Eigen vector
    Eigen::VectorXd eigenWeights =
        Eigen::Map<const Eigen::VectorXd>(weights.data(),
        static_cast<Eigen::Index>(weights.size()));

    // Compute the portfolio loss
    double portfolioLoss = riskContributions.dot(eigenWeights);
  }
}