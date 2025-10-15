//
// Computes Equal Risk Contribution (ERC) portfolio weights using
// Monte Carlo simulations with different methods, progressively increasing
// the dataset size (25%, 50%, 75%, 100%) and comparing results
// across multiple threads for efficiency.
// Results are averaged across threads and exported to a CSV file.
// ===========================================================

#include <string>
#include <iostream>
#include <vector>
#include <map>
#include <fstream>
#include <algorithm>
#include <iomanip>
#include <future>
#include <thread>

#include "quantdream/legacy/csvReader/CsvReader.h"
#include "quantdream/legacy/monteCarlo/engine.h"

// Data structure used to store Yahoo Finance data
// The first level is the date, the second is the category (like Close, Open, etc.),
// the third is the ticker symbol, and the value is the numeric observation.
using YFData = std::map<std::string,
                        std::map<std::string,
                        std::map<std::string, double>>>;

// Forward declarations of helper functions
void runProgressiveERC(const YFData& data, bool verbose = false);
void exportWeightsToCSV(
    const std::map<std::string, std::vector<std::vector<double>>>& weights,
    const std::string& filename,
    const std::vector<double>& fractions);

// Main program entry point
int main() {
  try {
    const std::string FILENAME = "../standalone/datasets/msci_portfolio.csv";

    // Read the dataset using the QuantDream CSV reader
    YFData data = getYFCSV(FILENAME);

    // Run the progressive ERC optimization
    // Set verbose to true if you want detailed output from each thread
    runProgressiveERC(data, false);
  }
  catch (const std::exception& ex) {
    std::cerr << "Error: " << ex.what() << std::endl;
    return 1;
  }
  return 0;
}

// Function that runs the progressive Equal Risk Contribution (ERC) optimization
// It increases the data sample size fractionally (25%, 50%, 75%, 100%) and
// compares different simulation methods in parallel.
void runProgressiveERC(const YFData& data, bool verbose) {
  // Fractions of the dataset to be used in progressive runs
  std::vector<double> fractions = {0.25, 0.5, 0.75, 1.0};

  // Define available simulation methods
  std::map<std::string, SimulationMethod> methods = {
      {"Vanilla", SimulationMethod::Vanilla},
      {"LambdaBias", SimulationMethod::LambdaBias},
      {"Stationary", SimulationMethod::Stationary}
  };

  // Optimizer configuration
  const size_t optimizerIters = 50;
  const double tol = 1e-3;
  const double eps_rc = 1e-10;
  const double damping = 0.5;

  // Parameters for each simulation type
  const size_t vanillaBlockSize = 15;
  const double lambdaBias = 0.5;
  const size_t stationaryBlockSize = 10;
  const double thetaTilt = 30.0;

  // Container to store weight results for all methods and dataset fractions
  std::map<std::string, std::vector<std::vector<double>>> weights;

  // Collect all dates available in the dataset
  std::vector<std::string> all_dates;
  for (auto &pair : data) all_dates.push_back(pair.first);
  std::sort(all_dates.begin(), all_dates.end());

  // General simulation parameters
  const size_t nSimulations = 1000;
  const size_t nSamples = 365 * 1;
  const size_t blockSize = 7;
  const size_t alpha = 5;

  // Determine the number of threads to use
  size_t nThreads = std::thread::hardware_concurrency();
  if (nThreads == 0) nThreads = 4;

  std::cout << "Launching " << nThreads
            << " Monte Carlo engines in parallel...\n\n";

  // Loop over each dataset fraction
  for (double frac : fractions) {
    size_t cutoff = static_cast<size_t>(frac * all_dates.size());
    if (cutoff < 2) continue;

    // Extract the subset of data up to the current fraction
    YFData sliced_data;
    for (size_t i = 0; i < cutoff; ++i)
      sliced_data[all_dates[i]] = data.at(all_dates[i]);

    std::cout << "Running ERC optimization with "
              << std::setw(5) << (frac * 100) << "% of dataset ("
              << cutoff << " samples)\n";

    // Create asynchronous jobs for each thread
    std::vector<std::future<std::map<std::string, std::vector<double>>>> futures;

    for (size_t t = 0; t < nThreads; ++t) {
      futures.emplace_back(std::async(std::launch::async, [&, t]() {
        MonteCarloEngine mc(sliced_data, nSimulations, nSamples, blockSize, alpha);
        mc.setSeed(std::random_device{}() + t);
        mc.selectCategory("Close");

        std::map<std::string, std::vector<double>> localResults;

        // Run each simulation method
        for (const auto& [name, method] : methods) {
          std::vector<double> w;
          if (method == SimulationMethod::Vanilla)
            w = mc.solveERC(optimizerIters, method, vanillaBlockSize, 0.0, tol, eps_rc, damping, false);
          else if (method == SimulationMethod::LambdaBias)
            w = mc.solveERC(optimizerIters, method, lambdaBias, 0.0, tol, eps_rc, damping, false);
          else
            w = mc.solveERC(optimizerIters, method, stationaryBlockSize, thetaTilt, tol, eps_rc, damping, false);

          localResults[name] = w;
        }
        return localResults;
      }));
    }

    // Aggregate results from all threads
    std::map<std::string, std::vector<double>> avgResults;
    for (auto &f : futures) {
      auto local = f.get();
      for (const auto& [name, vec] : local) {
        if (avgResults[name].empty()) avgResults[name].resize(vec.size(), 0.0);
        for (size_t i = 0; i < vec.size(); ++i)
          avgResults[name][i] += vec[i] / static_cast<double>(nThreads);
      }
    }

    // Store averaged weights and print output
    for (const auto& [name, vec] : avgResults) {
      weights[name].push_back(vec);
      if (verbose) {
        std::cout << "  " << name << " Weights: ";
        for (double wi : vec)
          std::cout << std::fixed << std::setprecision(4) << wi << " ";
        std::cout << "\n";
      }
    }

    // Print concise summary when not in verbose mode
    if (!verbose) {
      std::cout << "   Completed fraction " << (frac * 100) << "% "
                << "(averaged across " << nThreads << " threads)\n";
    }
  }

  // Export all results to CSV
  exportWeightsToCSV(weights, "erc_weight_evolution_parallel.csv", fractions);
  std::cout << "\nAll threads completed successfully. Results written to erc_weight_evolution_parallel.csv\n";
}

// This function exports all computed weights to a CSV file
// Each row contains the method name, the dataset fraction, the asset index, and the corresponding weight value.
void exportWeightsToCSV(
    const std::map<std::string, std::vector<std::vector<double>>>& weights,
    const std::string& filename,
    const std::vector<double>& fractions) {

  std::ofstream out(filename);
  if (!out.is_open()) {
    std::cerr << "Could not open " << filename << " for writing.\n";
    return;
  }

  out << "Method,Fraction,AssetIndex,Weight\n";
  for (const auto& [method, runs] : weights) {
    for (size_t f = 0; f < runs.size(); ++f)
      for (size_t i = 0; i < runs[f].size(); ++i)
        out << method << "," << fractions[f] << "," << i << "," << runs[f][i] << "\n";
  }

  out.close();
  std::cout << "Weights exported to " << filename << "\n";
}
