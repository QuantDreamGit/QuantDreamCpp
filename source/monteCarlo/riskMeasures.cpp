//
// Created by Giuseppe Priolo on 29/09/25.
//
#include <Eigen/Dense>
#include <vector>
#include <algorithm>
#include <stdexcept>
#include "monteCarlo/riskMeasures.h"
#include "monteCarlo/utils.h"

// Matrix structure:
// Cols: Loss_asset_0 | ... | Loss_asset_N-1 | Portfolio_Loss
// Rows: Simulation_0
//        .
//        .
//        .
// Rows: Simulation_M-1
std::vector<double> computePortfolioRiskMeasures(const std::vector<Eigen::MatrixXd> &simulatedReturns,
                                             const std::vector<double> &weights,
                                             const size_t &alpha,
                                             const RiskMeasure &measure,
                                             const bool plotLosses) {
  // Get the dimension of the output matrix
  const size_t nSimulations = simulatedReturns.size();
  const size_t nAssets = weights.size();

  // Construct the random variable
  Eigen::MatrixXd riskMeasureMatrix(nSimulations, nAssets + 1);

  // Select the correct function to compute the risk measure
  // auto riskFunction = (measure == RiskMeasure::VaR) ? computeVaR : computeES;

  // For each simulation, compute the portfolio loss and the losses of each asset
  for (size_t i = 0; i < nSimulations; i++) {
    // Compute the portfolio losses of the i-th simulation
    // To compute the loss, we can use the formula:
    // loss_j = prod (1 + return_ij)
    // total_loss_j = 1 - prod (1 + loss_j)
    // Transposition is needed to have a column vector of losses
    Eigen::VectorXd losses = (1 - (simulatedReturns[i].array() + 1).colwise().prod()).transpose();

    // Convert weights to Eigen vector
    Eigen::VectorXd eigenWeights =
        Eigen::Map<const Eigen::VectorXd>(weights.data(),
        static_cast<Eigen::Index>(weights.size()));

    // Compute the portfolio loss
    double portfolioLoss = losses.dot(eigenWeights);

    // Insert into riskMeasureMatrix losses
    riskMeasureMatrix.row(i).head(nAssets) = losses;
    riskMeasureMatrix(i, nAssets) = portfolioLoss;
  }

  if (plotLosses) { plotPortfolioLosses(riskMeasureMatrix); }

  // Order portfolio losses in increasing order
  // and get the index of the alpha-quantile
  // Fill portfolioLosses and indices
  std::vector<double> portfolioLosses(nSimulations);
  std::vector<std::size_t> indices(nSimulations);

  for (Eigen::Index i = 0; i < nSimulations; ++i) {
    portfolioLosses[i] = riskMeasureMatrix(i, static_cast<Eigen::Index>(nAssets));
    indices[i] = i;
  }

  // Sort indices based on the values in portfolioLosses
  std::sort(indices.begin(), indices.end(),
    // [&] is used to capture outer scope variables by reference
    [&](size_t a, size_t b) {
      return portfolioLosses[a] < portfolioLosses[b];
    });

  // Retrieve alpha-quantile index
  auto quantileIndex = static_cast<size_t>(std::floor((1 - alpha / 100.0) * nSimulations));
  if (quantileIndex >= nSimulations) quantileIndex = nSimulations - 1;

  std::vector<double> results(nAssets + 1);
  if (measure == RiskMeasure::VaR) {
    for (size_t j = 0; j < nAssets + 1; ++j) {
      // Compute the VaR
      results[j] = riskMeasureMatrix(indices[quantileIndex], j);

      if (j < nAssets) {
        // Compute the marginal VaR for each asset
        results[j] *= weights[j];
      }
    }
  }

  if (measure == RiskMeasure::ES) {
    for (size_t i = quantileIndex; i < nSimulations; ++i) {
      for (size_t j = 0; j < nAssets + 1; ++j) {
        results[j] += riskMeasureMatrix(indices[i], j);
      }
    }

    for (size_t j = 0; j < nAssets + 1; ++j) {
      results[j] /= static_cast<double>(nSimulations - quantileIndex);

      if (j < nAssets) {
        // Compute the marginal ES for each asset
        results[j] *= weights[j];
      }
    }
  }

  return results;
}