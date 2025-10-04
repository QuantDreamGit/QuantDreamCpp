//
// Created by user on 9/30/25.
//

#ifndef QUANTDREAMCPP_ERCOPTIMIZER_H
#define QUANTDREAMCPP_ERCOPTIMIZER_H

#include <vector>
#include "engine.h"

class ERCOptimizer {
public:
  ERCOptimizer(MonteCarloEngine& mc,
               size_t nAssets,
               size_t nMaxIterations,
               SimulationMethod simMethod,
               double param1,
               double param2);

  std::vector<double> optimize(
    double tol = 1e-4,            // relative tolerance on RC dispersion (vs ES)
    double eps_rc = 1e-10,        // floor to avoid division by ~0
    double damping = 0.5,         // 0<damping<=1 (1=no damping). 0.3–0.7 helps stability
    bool verbose = true) const;   // print progress


private:
  MonteCarloEngine& mc_;
  size_t nAssets_;
  size_t nMaxIterations_;
  SimulationMethod simMethod_;
  double param1_;
  double param2_;
};


#endif  // QUANTDREAMCPP_ERCOPTIMIZER_H
