//
// Created by user on 9/24/25.
//
// Description:
//   Example program demonstrating how to load a CSV file into the YFinance
//   data structure, run different Monte Carlo simulations, and solve for an
//   Equal Risk Contribution (ERC) portfolio.
//
//   Supported simulation methods:
//     - Vanilla bootstrap
//     - Lambda-bias bootstrap
//     - Stationary bootstrap with tilt
//

#include <string>
#include <iostream>
#include <vector>

#include "quantdream/legacy/csvReader/CsvReader.h"
#include "quantdream/legacy/monteCarlo/engine.h"
#include "quantdream/legacy/monteCarlo/riskMeasures.h"

// YFinance Data Structure
using YFData = std::map<std::string,                      // Date
                        std::map<std::string,             // Category
                        std::map<std::string,             // Ticker
                        double>>>;                        // Value

int main() {
  try {
    // ---------------------------------------------------------
    // Step 1: Load CSV file into YFData structure
    // ---------------------------------------------------------
    const std::string FILENAME = "../standalone/datasets/msci_portfolio.csv";
    YFData data = getYFCSV(FILENAME);

    // ---------------------------------------------------------
    // Step 2: Initialize Monte Carlo Engine
    // ---------------------------------------------------------
    const size_t nSimulations = 1000;  // number of Monte Carlo replications
    const size_t nSamples     = 365;   // horizon length (1 year daily)
    const size_t blockSize    = 7;     // default block size
    const size_t alpha        = 5;     // ES/VaR confidence level (%)
    // ---------------------------------------------------------
    // ERC optimization parameters
    // ---------------------------------------------------------
    const size_t optimizerIters = 50;  // max ERC iterations
    double tol = 1e-4;                 // relative tolerance on RiskContribution dispersion (vs ES)
    double eps_rc = 1e-10;             // floor to avoid division by ~0
    double damping = 0.5;              // 0<damping<=1 (1=no damping). 0.3–0.7 helps stability
    bool verbose = false;               // print progress

    const size_t vanillaBlockSize = 10;
    const double lambdaBias       = 0.7;

    const size_t stationaryBlockSize = 10;
    const double thetaTilt           = 30.0;

    MonteCarloEngine mc(data, nSimulations, nSamples, blockSize, alpha);
    mc.setSeed(420);
    mc.selectCategory("Close");  // use close prices

    // ---------------------------------------------------------
    // Step 2a: Simulations statistics
    // ---------------------------------------------------------
    //
    // Description:
    // Run a simulation and compute risk contributions
    // (marginal ES) for each asset in the portfolio.

    // Vanilla case
    mc.runSimulation(SimulationMethod::Vanilla, blockSize, 0.0);
    auto rc = mc.computeRiskContributions(RiskMeasure::ES, true);
    std::cout << "Vanilla Portfolio Expected Shortfall (ES): " << mc.getPortfolioLoss() << "\n";

    // Lambda-bias case
    mc.runSimulation(SimulationMethod::LambdaBias, lambdaBias, 0.0);
    rc = mc.computeRiskContributions(RiskMeasure::ES, true);
    std::cout << "Vanilla Portfolio Expected Shortfall (ES): " << mc.getPortfolioLoss() << "\n";

    // Stationary case
    mc.runSimulation(SimulationMethod::Stationary, stationaryBlockSize, thetaTilt);
    rc = mc.computeRiskContributions(RiskMeasure::ES, true);
    std::cout << "Vanilla Portfolio Expected Shortfall (ES): " << mc.getPortfolioLoss() << "\n";


    // ---------------------------------------------------------
    // Step 3a: Vanilla Bootstrap
    // ---------------------------------------------------------
    //
    // Description:
    //   Standard block bootstrap of returns, preserving cross-sectional
    //   correlation across assets but sampling blocks uniformly at random.
    //
    // Parameters:
    //   - blockSize (int > 0):
    //       Length of contiguous blocks of returns resampled.
    //       Larger values preserve more autocorrelation structure.
    //       Smaller values make paths more "scrambled."
    //
    // Boundaries:
    //   blockSize >= 1 and blockSize <= sample length.
    //

    std::cout << "\n=== ERC with Vanilla Bootstrap (block size = "
              << vanillaBlockSize << ") ===\n";
    auto w_vanilla = mc.solveERC(optimizerIters,
                                 SimulationMethod::Vanilla,
                                 vanillaBlockSize, 0.0,
                                 tol, eps_rc, damping, verbose);
    for (size_t i = 0; i < w_vanilla.size(); ++i) {
      std::cout << "Asset " << i << ": " << w_vanilla[i] << "\n";
    }

    // ---------------------------------------------------------
    // Step 3b: Lambda-Bias Bootstrap
    // ---------------------------------------------------------
    //
    // Description:
    //   Modified bootstrap that biases block selection toward
    //   "bad states" (negative portfolio returns).
    //   Provides more stress scenarios than uniform sampling.
    //
    // Parameters:
    //   - lambda (0.0 ≤ λ ≤ 1.0):
    //       Controls tilt toward losses.
    //       λ = 0.0 → pure uniform bootstrap (no bias).
    //       λ = 1.0 → always favor worst losses.
    //
    std::cout << "\n=== ERC with Lambda-Bias Bootstrap (lambda = "
              << lambdaBias << ") ===\n";
    auto w_lambda = mc.solveERC(optimizerIters,
                                SimulationMethod::LambdaBias,
                                lambdaBias, 0.0,
                                tol, eps_rc, damping, verbose);
    for (size_t i = 0; i < w_lambda.size(); ++i) {
      std::cout << "Asset " << i << ": " << w_lambda[i] << "\n";
    }

    // ---------------------------------------------------------
    // Step 3c: Stationary Bootstrap with Tilt
    // ---------------------------------------------------------
    //
    // Description:
    //   Stationary bootstrap (Politis & Romano, 1994) with exponential tilt
    //   toward losses. Generates random block lengths (geometric distribution),
    //   which avoids "edge effects" and produces smoother resamples.
    //
    // Parameters:
    //   - blockSizeMean (int > 0):
    //       Expected block length (mean of geometric distribution).
    //       Controls persistence of local time structure.
    //
    //   - theta (θ ≥ 0):
    //       Loss-tilt parameter.
    //       θ = 0.0 → uniform stationary bootstrap.
    //       Larger θ exponentially increases probability of selecting
    //       blocks with large portfolio losses.
    //
    // Boundaries:
    //   blockSizeMean ≥ 1, theta ≥ 0.
    //
    std::cout << "\n=== ERC with Stationary Bootstrap "
              << "(mean block = " << stationaryBlockSize
              << ", theta = " << thetaTilt << ") ===\n";
    auto w_stationary = mc.solveERC(optimizerIters,
                                    SimulationMethod::Stationary,
                                    stationaryBlockSize, thetaTilt,
                                    tol, eps_rc, damping, verbose);
    for (size_t i = 0; i < w_stationary.size(); ++i) {
      std::cout << "Asset " << i << ": " << w_stationary[i] << "\n";
    }

  } catch (const std::exception& ex) {
    std::cerr << "Error: " << ex.what() << std::endl;
    return 1;
  }

  return 0;
}
