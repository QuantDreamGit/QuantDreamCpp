//
// Created by user on 9/30/25.
//

#include "quantdream/legacy/monteCarlo/ERCOptimizer.h"

#include <memory>
#include <vector>
#include <iostream>
#include <numeric>
#include <stdexcept>

ERCOptimizer::ERCOptimizer(MonteCarloEngine &mc,
                           const size_t nAssets,
                           const size_t nMaxIterations,
                           SimulationMethod simMethod,
                           double param1,
                           double param2)
    : mc_(mc),
      nAssets_(nAssets),
      nMaxIterations_(nMaxIterations),
      simMethod_(simMethod),
      param1_(param1),
      param2_(param2) {}

std::vector<double> ERCOptimizer::optimize(
  // hyperparameters for the multiplicative update
  const double tol,       // relative tolerance on RC dispersion (vs ES)
  const double eps_rc,    // floor to avoid division by ~0 and handle negatives
  const double damping,   // 0<damping<=1 (1=no damping). 0.3–0.7 helps stability
  const bool verbose      // print progress
) const {
    // --- initialization: start from equal weights ---
    std::vector<double> w(nAssets_, 1.0 / static_cast<double>(nAssets_));



    for (size_t iter = 0; iter < nMaxIterations_; ++iter) {
    // --- progress bar (always shown) ---
    double progress = (100.0 * (iter + 1)) / static_cast<double>(nMaxIterations_);
    int barWidth = 50; // characters in the bar
    int pos = static_cast<int>(barWidth * progress / 100.0);

    std::cout << "[";
    for (int i = 0; i < barWidth; ++i) {
        if (i < pos) std::cout << "=";
        else if (i == pos) std::cout << ">";
        else std::cout << " ";
    }
    std::cout << "] " << int(progress) << " %\r";
    std::cout.flush();

    // --- resimulate scenarios ---
    mc_.setWeights(w);
    mc_.runSimulation(simMethod_, param1_, param2_);

    // --- compute RCs and ES ---
    std::vector<double> rc = mc_.computeRiskContributions(RiskMeasure::ES);
    if (rc.size() != nAssets_) {
        throw std::runtime_error("ERCOptimizer: RC size mismatch (expected nAssets).");
    }
    double ES = mc_.getPortfolioLoss();
    if (!(ES >= 0.0)) ES = std::abs(ES);

    const double target = (nAssets_ > 0 ? ES / static_cast<double>(nAssets_) : 0.0);

    // max relative deviation
    double max_dev = 0.0;
    for (size_t i = 0; i < nAssets_; ++i) {
        max_dev = std::max(max_dev, std::abs(rc[i] - target));
    }
    const double rel_dev = (ES > 0.0 ? max_dev / ES : max_dev);

    // --- optional verbose diagnostics ---
    if (verbose) {
        std::cout << "\nIter " << iter
                  << " | ES=" << ES
                  << " | targetRC=" << target
                  << " | maxDev/ES=" << rel_dev
                  << "\nRC: ";
        for (double rci : rc) std::cout << rci << " ";
        std::cout << "\nWeights:";
        for (double wi : w) std::cout << " " << wi;
        std::cout << "\n";
    }

    // --- convergence check ---
    if (rel_dev <= tol) {
        if (verbose) {
            std::cout << "\nERC converged (rel dev <= " << tol << ")\n";
            std::cout << "Final Weights:";
            for (double wi : w) std::cout << " " << wi;
            std::cout << "\n";
        }
        break;
    }

    // --- multiplicative update ---
    std::vector<double> w_prop(nAssets_);
    for (size_t i = 0; i < nAssets_; ++i) {
        const double denom = std::max(rc[i], eps_rc);
        w_prop[i] = w[i] * (target / denom);
        if (w_prop[i] < 0.0) w_prop[i] = 0.0;
    }

    double sum_w = 0.0;
    for (double wi : w_prop) sum_w += wi;
    if (sum_w <= 0.0) {
        for (size_t i = 0; i < nAssets_; ++i) w_prop[i] = 1.0 / static_cast<double>(nAssets_);
    } else {
        for (size_t i = 0; i < nAssets_; ++i) w_prop[i] /= sum_w;
    }

    for (size_t i = 0; i < nAssets_; ++i) {
        w[i] = (1.0 - damping) * w[i] + damping * w_prop[i];
    }

    double sum_after = 0.0;
    for (double wi : w) sum_after += wi;
    if (sum_after != 0.0) {
        for (double &wi : w) wi /= sum_after;
    }
}

// make sure bar ends cleanly
std::cout << std::endl;

    return w;
}
