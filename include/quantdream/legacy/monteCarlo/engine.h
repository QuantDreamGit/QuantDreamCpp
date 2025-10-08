#ifndef QUANTDREAMCPP_ENGINE_H
#define QUANTDREAMCPP_ENGINE_H

#include "riskMeasures.h"

#include <map>
#include <string>
#include <random>
#include <vector>
#include <Eigen/Dense>

// YFinance Data Structure
using YFData = std::map<std::string,                      // Date
                        std::map<std::string,             // Category
                        std::map<std::string,             // Ticker
                        double>>>;

// Create a map to hold the selected data of each ticker
// To get faster access, we will use a vector of doubles
// It will be a map of the tickers containing a vector of values
using SelectedData = std::map<std::string,                // Ticker
                     std::vector<double>>;                // Value

enum class SimulationMethod {
  Vanilla,      // Uniform block bootstrap
  LambdaBias,   // Badness-weighted bootstrap
  Stationary    // Stationary bootstrap with optional tilt
};

class MonteCarloEngine {
public:
  MonteCarloEngine(YFData data,
                   const size_t &nSimulations,
                   const size_t &nSamples,
                   const size_t &blockSize,
                   const size_t &alpha);

  // --- Data selection ---
  void selectCategory(const std::string &category);

  // --- Simulation methods ---
  Eigen::MatrixXd runSingleSimulationVanilla(size_t blockSize);
  Eigen::MatrixXd runSingleSimulation(size_t blockSize, double lambda = 0.0);

  // New: stationary bootstrap with optional exponential tilt
  // blockSizeMean = average block length (geometric distribution)
  // theta = tilt severity (0.0 = uniform, >0 favors losses)
  Eigen::MatrixXd runSingleSimulationStationary(size_t blockSizeMean, double theta = 0.0);

  // Run multiple simulations and store results
  void runSimulation(SimulationMethod method,
                     double param1 = 0.0,
                     double param2 = 0.0);

  // --- Portfolio weights ---
  [[nodiscard]] std::vector<double> getWeights() const { return weightsVector_; }
  void setWeights(const std::vector<double> &weightsVector);

  // --- Risk measures ---
  std::vector<double> computeRiskContributions(RiskMeasure measure, bool plotLosses = false);
  std::vector<double> solveERC(const size_t maxIterations,
                               const SimulationMethod simMethod,
                               const double param1,
                               const double param2,
                               const double tol,            // relative tolerance on RC dispersion (vs ES)
                               const double eps_rc,         // floor to avoid division by ~0
                               const double damping,        // 0<damping<=1 (1=no damping). 0.3–0.7 helps stability
                               const bool verbose);

  [[nodiscard]] std::vector<double> getRiskContributions() const { return riskContributions_; }
  [[nodiscard]] double getPortfolioLoss() const {
    if (riskContributions_.empty()) return 0.0;
    return riskContributions_.back();
  }

  // --- RNG control ---
  void setSeed(const size_t &seed) { rng_.seed(seed); }

private:
  // Define parameters
  std::mt19937 rng_;
  size_t nSimulations_;
  size_t nSamples_;
  size_t alpha_;
  size_t blockSize_ = 1;

  // Define private members
  YFData marketData_;
  SelectedData selectedData_;
  Eigen::MatrixXd selectedDataReturns_;         // Size (T-1, N): N tickers, T time points
  std::vector<Eigen::MatrixXd> simulatedDataReturns_;
  std::vector<std::string> availableTickers_;
  std::vector<double> weightsVector_;
  std::vector<std::string> weightsTickers_;
  std::vector<double> portfolioReturns_;
  std::vector<double> riskContributions_;

  // Private methods
  void setInitialWeights_();
  void computeSelectedDataReturns_();
};

#endif  // QUANTDREAMCPP_ENGINE_H
