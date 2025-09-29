//
// Created by user on 9/24/25.
//

#include "monteCarlo/engine.h"
#include "monteCarlo/riskMeasures.h"

#include <map>
#include <random>
#include <string>
#include <utility>
#include <vector>
#include <stdexcept>
#include <iterator>
#include <cmath>

// YFinance Data Structure
using YFData = std::map<std::string,                      // Date
                        std::map<std::string,             // Category
                        std::map<std::string,             // Ticker
                        double>>>;                        // Value

MonteCarloEngine::MonteCarloEngine(YFData data,
                                   const size_t &nSimulations,
                                   const size_t &nSamples,
                                   const size_t &alpha)
                                     : marketData_(std::move(data)),
                                       nSimulations_(nSimulations),
                                       nSamples_(nSamples),
                                       alpha_(alpha),
                                       rng_(std::random_device{}()) {}

void MonteCarloEngine::setInitialWeights_() {
  // Get the number of assets from the first date
  size_t N = (selectedData_.begin()->second).size();
  double weight = 1.0 / static_cast<double>(N);
  for (size_t i = 0; i < weightsTickers_.size(); i++) {
    weightsVector_.push_back(weight);
  }
}

void MonteCarloEngine::computeSelectedDataReturns_() {
  // Initialize the matrix of returns
  const size_t cols = selectedData_.size();
  const size_t rows = (selectedData_.begin()->second).size() - 1;
  selectedDataReturns_.resize(rows, cols);

  // It's executed after selectCategory, so no need to check if selectedData_ is empty
  // Iterate over each ticker in selectedData_
  size_t j = 0;
  for (const auto &[ticker, values] : selectedData_) {
    for (size_t i = 1; i < values.size(); ++i) {
      // Compute simple returns
      const double ret = (values[i] - values[i - 1]) / values[i - 1];
        selectedDataReturns_(i - 1, j) = ret;
    }
    ++j;
  }
}

void MonteCarloEngine::selectCategory(const std::string &category) {
  if (marketData_.empty()) {
    throw std::runtime_error("Market data is empty! Please check input data before selecting a category.");
  }

  // Keep only the selected category in marketData_
  for (const auto &[date, categories] : marketData_) {
    // Create an iterator to find the category
    auto it = categories.find(category);

    // Check if the category exists
    if (it != categories.end()) {
      // Check if any ticker has NaN values
      // If so, skip the entire date
      bool hasNaN = false;

      if (auto innerIt = it->second.begin(); innerIt != it->second.end()) {
        for (const auto &[ticker, value] : it->second) {
          if (std::isnan(value)) {
            hasNaN = true;
            break;
          }
        }
      } else {
        hasNaN = true; // No tickers available
      }
      if (hasNaN) {
        continue; // Skip this date
      }

      for (auto &[ticker, value] : it->second) {
        // Save the selected data
        selectedData_[ticker].push_back(value);
      }

    } else {
      // Return error if category not found
      throw std::runtime_error("Category not found in data! Please check the category name.");
    }
  }

  // Fill availableTickers_
  availableTickers_.clear();
  for (const auto &[ticker, _] : selectedData_) {
    availableTickers_.push_back(ticker);
  }

  if (!selectedData_.empty()) {
    // Compute returns for each ticker
    computeSelectedDataReturns_();

    // Set initial weights to 1 / N
    // where N is the number of assets
    setInitialWeights_();
  } else {
    throw std::runtime_error("Selected data is empty after selecting category! Please check input data.");
  }
}

Eigen::MatrixXd MonteCarloEngine::runSingleSimulation() {
  if (selectedDataReturns_.size() == 0) {
    throw std::runtime_error("No category selected! Please select a category before running simulation."
                             "Use selectCategory() method.");
  }

  // Generate uniform distribution
  const size_t T = selectedDataReturns_.rows();
  const size_t n_cols = selectedDataReturns_.cols();
  std::uniform_int_distribution<size_t> distribution(0, T - 1);

  // Initialize portfolio and assets' losses
  Eigen::MatrixXd simulatedReturns(nSamples_, n_cols);

  // Extract an index from the distribution
  // Then Pick the corresponding return from each asset so that cross-correlation is kept
  for (size_t row = 0; row < nSamples_; row++) {
    size_t idx = distribution(rng_);

    // Compute the loss of each asset in the portfolio
    for (size_t col = 0; col < n_cols; col++) {
      simulatedReturns(row, col) = selectedDataReturns_(idx, col);
    }
  }

  return simulatedReturns;
}

std::vector<Eigen::MatrixXd> MonteCarloEngine::runSimulation(const RiskMeasure measure) {
  // Clear previous results
  simulatedDataReturns_.clear();

  // Run multiple single runs of the simulation
  // Then store the results in a vector so that Eigen matrices can be used
  for (size_t i = 0; i < nSimulations_; ++i) {
    simulatedDataReturns_.push_back(runSingleSimulation());
  }

  computePortfolioRiskMeasures(simulatedDataReturns_, weightsVector_, alpha_, RiskMeasure::VaR);

  return simulatedDataReturns_;
}

