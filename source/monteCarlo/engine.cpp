//
// Created by user on 9/24/25.
//

#include "monteCarlo/engine.h"
#include "monteCarlo/riskMeasures.h"
#include "monteCarlo/ERCOptimizer.h"

#include <eigen3/Eigen/Dense>
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
                                   const size_t &blockSize,
                                   const size_t &alpha)
                                     : marketData_(std::move(data)),
                                       nSimulations_(nSimulations),
                                       nSamples_(nSamples),
                                       alpha_(alpha),
                                       rng_(std::random_device{}()) {}

void MonteCarloEngine::setInitialWeights_() {
  // Get the number of assets from the first date
  size_t N = selectedData_.size();
  double weight = 1.0 / static_cast<double>(N);
  for (size_t i = 0; i < N; i++) {
    weightsVector_.push_back(weight);
  }
}

void MonteCarloEngine::setWeights(const std::vector<double> &weightsVector) {
  if (weightsVector.size() != availableTickers_.size()) {
    throw std::runtime_error("Weights vector size does not match number of available tickers!");
  }

  double sum = 0.0;
  for (const auto &w : weightsVector) {
    if (w < 0.0) {
      throw std::runtime_error("Weights must be non-negative!");
    }
    sum += w;
  }

  if (std::abs(sum - 1.0) > 1e-6) {
    throw std::runtime_error("Weights must sum to 1!");
  }

  weightsVector_ = weightsVector;
}

void MonteCarloEngine::computeSelectedDataReturns_() {
  // Initialize the matrix of returns
  const size_t cols = selectedData_.size();
  const size_t rows = (selectedData_.begin()->second).size() - 1;
  selectedDataReturns_.resize(rows, cols);

  size_t j = 0;
  for (const auto &[ticker, values] : selectedData_) {
    std::vector<double> returns;
    std::vector<double> x_price(values.size()), x_ret(rows);

    // x-axis for prices
    for (size_t i = 0; i < values.size(); ++i) {
      x_price[i] = i;
    }

    // Compute returns
    for (size_t i = 1; i < values.size(); ++i) {
      const double ret = (values[i] - values[i - 1]) / values[i - 1];
      selectedDataReturns_(i - 1, j) = ret;
      returns.push_back(ret);
      x_ret[i - 1] = i - 1;
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

Eigen::MatrixXd MonteCarloEngine::runSingleSimulationVanilla(const size_t blockSize) {
  if (selectedDataReturns_.size() == 0) {
    throw std::runtime_error("No category selected! Please select a category before running simulation."
                             "Use selectCategory() method.");
  }

  // Generate uniform distribution
  const size_t T = selectedDataReturns_.rows() - blockSize;
  const size_t n_cols = selectedDataReturns_.cols();
  std::uniform_int_distribution<size_t> distribution(0, T - 1);

  // Initialize portfolio and assets' losses
  Eigen::MatrixXd simulatedReturns(nSamples_, n_cols);

  // Extract an index from the distribution
  // Then Pick the corresponding return from each asset so that cross-correlation is kept
  for (Eigen::Index row = 0; row < nSamples_ / blockSize + 1; row++) {
    auto idx = static_cast<Eigen::Index>(distribution(rng_));
    for (Eigen::Index block = 0; block < blockSize; block++) {
      if (row * blockSize + block >= nSamples_) break;
      for (Eigen::Index col = 0; col < n_cols; col++) {
        simulatedReturns(row * blockSize + block, col) =
            selectedDataReturns_(idx + block, col);
      }
    }
  }

  return simulatedReturns;
}

Eigen::MatrixXd MonteCarloEngine::runSingleSimulation(const size_t blockSize, const double lambda) {
  // Safety: make sure returns have been computed
  if (selectedDataReturns_.size() == 0) {
    throw std::runtime_error(
        "No category selected! Please select a category before running simulation. "
        "Use selectCategory() method.");
  }

  // Total number of possible starting points for a block
  // Example: if you have 1000 rows and blockSize=10, you can start at 0..990
  const size_t T = selectedDataReturns_.rows() - blockSize + 1;
  const size_t n_cols = selectedDataReturns_.cols();

  // If no weights are set, use equal weights across assets
  std::vector<double> w = weightsVector_;
  if (w.empty()) {
    w.assign(n_cols, 1.0 / n_cols);
  }
  Eigen::Map<const Eigen::VectorXd> ew(w.data(), w.size());

  // ---------------------------------------------------------
  // Step 1: Compute "badness scores" for each block start
  // ---------------------------------------------------------
  // Idea:
  //   - If portfolio return at time t is negative, give higher score.
  //   - Mix with uniform distribution so you still sample good states.
  //
  // λ = 0.0 → pure uniform bootstrap (no bias).
  // λ = 1.0 → pure badness-driven bootstrap (always favor losses).
  // Values in between blend the two.
  // ---------------------------------------------------------
  std::vector<double> score(T);
  for (size_t t = 0; t < T; ++t) {
    // Portfolio one-step return at row t
    double port_r = selectedDataReturns_.row(t).dot(ew);

    // Loss proxy: only care if return is negative
    double loss_val = std::max(0.0, -port_r);

    // Quadratic penalty (large losses weigh more than small ones)
    double badness = std::pow(loss_val, 2);

    // Blend with uniform baseline
    // Ensures even good states have nonzero probability
    score[t] = lambda * badness + (1.0 - lambda);
  }

  // Normalize scores into probabilities
  double Z = std::accumulate(score.begin(), score.end(), 0.0);
  std::vector<double> prob(T);
  for (size_t t = 0; t < T; ++t) prob[t] = score[t] / Z;

  // Build discrete distribution from probabilities
  std::discrete_distribution<size_t> distribution(prob.begin(), prob.end());

  // ---------------------------------------------------------
  // Step 2: Generate one bootstrap simulation
  // ---------------------------------------------------------
  Eigen::MatrixXd simulatedReturns(nSamples_, n_cols);

  size_t filled = 0;
  while (filled < nSamples_) {
    // Pick a random block start (biased by prob[] scores)
    size_t idx = distribution(rng_);

    // Copy blockSize rows starting at idx
    for (size_t b = 0; b < blockSize && filled < nSamples_; ++b) {
      simulatedReturns.row(filled) = selectedDataReturns_.row(idx + b);
      ++filled;
    }
  }

  return simulatedReturns;
}

Eigen::MatrixXd MonteCarloEngine::runSingleSimulationStationary(const size_t blockSizeMean,
                                                                const double theta) {
  if (selectedDataReturns_.size() == 0) {
    throw std::runtime_error(
        "No category selected! Please select a category before running simulation. "
        "Use selectCategory() method.");
  }

  const size_t N = selectedDataReturns_.rows();       // total observations
  const size_t n_cols = selectedDataReturns_.cols();  // number of assets
  if (N == 0) throw std::runtime_error("Empty returns matrix.");

  // If no weights are set, use equal weights across assets
  std::vector<double> w = weightsVector_;
  if (w.empty()) w.assign(n_cols, 1.0 / n_cols);
  Eigen::Map<const Eigen::VectorXd> ew(w.data(), w.size());

  // ---------------------------------------------------------
  // Step 1: compute tilted probabilities over start indices
  // ---------------------------------------------------------
  // Portfolio one-step return used as proxy for "badness"
  // theta = 0.0 → uniform (no tilt)
  // theta > 0.0 → exponentially favors large losses
  std::vector<double> score(N);
  for (size_t t = 0; t < N; ++t) {
    double port_r = selectedDataReturns_.row(static_cast<Eigen::Index>(t)).dot(ew);
    double loss_val = std::max(0.0, -port_r);
    score[t] = std::exp(theta * loss_val);
  }

  double Z = std::accumulate(score.begin(), score.end(), 0.0);
  if (Z <= 0.0) Z = 1.0;
  std::vector<double> prob(N);
  for (size_t t = 0; t < N; ++t) prob[t] = score[t] / Z;

  std::discrete_distribution<size_t> start_dist(prob.begin(), prob.end());

  // ---------------------------------------------------------
  // Step 2: stationary bootstrap parameters
  // ---------------------------------------------------------
  // Block lengths are random ~ Geometric(p), mean length = blockSizeMean
  double p = (blockSizeMean > 0) ? (1.0 / static_cast<double>(blockSizeMean)) : 1.0;
  if (p < 1e-9) p = 1e-9;
  if (p > 1.0) p = 1.0;
  std::geometric_distribution<size_t> geom(p);

  // Helper: circular row access
  auto getRow = [&](size_t t) -> Eigen::RowVectorXd {
    return selectedDataReturns_.row(static_cast<Eigen::Index>(t % N));
  };

  // ---------------------------------------------------------
  // Step 3: generate one bootstrap path
  // ---------------------------------------------------------
  Eigen::MatrixXd simulatedReturns(nSamples_, n_cols);
  size_t filled = 0;

  while (filled < nSamples_) {
    size_t idx0 = start_dist(rng_);  // pick starting index (tilted if theta > 0)
    size_t L = geom(rng_) + 1;       // block length ≥ 1
    if (L > nSamples_) L = nSamples_;

    for (size_t b = 0; b < L && filled < nSamples_; ++b) {
      simulatedReturns.row(static_cast<Eigen::Index>(filled)) = getRow(idx0 + b);
      ++filled;
    }
  }

  return simulatedReturns;
}

void MonteCarloEngine::runSimulation(SimulationMethod method,
                                     double param1,
                                     double param2) {
  // Clear previous results
  simulatedDataReturns_.clear();

  for (size_t i = 0; i < nSimulations_; ++i) {
    Eigen::MatrixXd sim;

    switch (method) {
      case SimulationMethod::Vanilla:
        // param1 = block size
        sim = runSingleSimulationVanilla(static_cast<size_t>(param1 > 0 ? param1 : blockSize_));
        break;

      case SimulationMethod::LambdaBias:
        // param1 = lambda
        sim = runSingleSimulation(blockSize_, param1);
        break;

      case SimulationMethod::Stationary:
        // param1 = mean block size, param2 = theta
        sim = runSingleSimulationStationary(static_cast<size_t>(param1 > 0 ? param1 : blockSize_),
                                            param2);
        break;

      default:
        throw std::runtime_error("Unknown simulation method");
    }

    simulatedDataReturns_.push_back(std::move(sim));
  }
}

std::vector<double> MonteCarloEngine::computeRiskContributions(const RiskMeasure measure, bool plotLosses) {
  if (simulatedDataReturns_.empty()) {
    throw std::runtime_error("No simulation run! Please run simulation before computing risk contributions."
                             "Use runSimulation() method.");
  }

  // Get the matrix of portfolio losses, including marginal ones
  riskContributions_ = computePortfolioRiskMeasures(
      simulatedDataReturns_, weightsVector_, alpha_, measure, plotLosses);

  // The method returns only the vector of risk contributions without the portfolio one
  return std::vector<double>(riskContributions_.begin(), riskContributions_.end() - 1);
}

std::vector<double> MonteCarloEngine::solveERC(const size_t maxIterations,
                                               const SimulationMethod simMethod,
                                               const double param1,
                                               const double param2,
                                               const double tol,            // relative tolerance on RC dispersion (vs ES)
                                               const double eps_rc,         // floor to avoid division by ~0
                                               const double damping,        // 0<damping<=1 (1=no damping). 0.3–0.7 helps stability
                                               const bool verbose) {
  // Initialize the optimizer with the chosen simulation method + parameters
  ERCOptimizer optimizer(*this,
                         availableTickers_.size(),
                         maxIterations,
                         simMethod,
                         param1,
                         param2);

  // Run the optimization
  std::vector<double> optimalWeights = optimizer.optimize(
    tol, eps_rc, damping, verbose
  );

  return optimalWeights;
}
