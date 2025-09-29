//
// Created by user on 9/24/25.
//5/5/2010

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
// This is because complexity of map is O(log N) while vector is O(1) for access by index
// It will be a map of the tickers containing a vector of values
using SelectedData = std::map<std::string,                // Ticker
                     std::vector<double>>;                // Value

class MonteCarloEngine {
public:
  MonteCarloEngine(YFData data,
                   const size_t &nSimulations,
                   const size_t &nSamples,
                   const size_t &alpha);

  // Main function to run Monte Carlo simulation
  void selectCategory(const std::string &category);
  Eigen::MatrixXd runSingleSimulation();
  std::vector<Eigen::MatrixXd> runSimulation(const RiskMeasure measure);

  void setSeed(const size_t &seed) { rng_.seed(seed); }
private:
  // Define parameters
  std::mt19937 rng_;
  size_t nSimulations_;
  size_t nSamples_;
  size_t alpha_;

  // Define private members
  YFData marketData_;
  SelectedData selectedData_;
  // To speed up calculations, we will store the returns in an Eigen matrix
  // Size (T-1, N)
  // where N is the number of tickers and T is the number of time points
  Eigen::MatrixXd selectedDataReturns_;
  std::vector<Eigen::MatrixXd> simulatedDataReturns_;

  // Store ordered available tickers
  std::vector<std::string> availableTickers_;
  // Set initial weights to 1 / N
  // where N is the number of assets
  std::vector<double> weightsVector_;
  std::vector<std::string> weightsTickers_;

  // Simulation Results
  std::vector<double> portfolioReturns_;
  Eigen::MatrixXd riskContributions_;

  // Private methods
  void setInitialWeights_();
  void computeSelectedDataReturns_();
};

#endif  // QUANTDREAMCPP_ENGINE_H
