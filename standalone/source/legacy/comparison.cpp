//
// Monte Carlo portfolio comparison (Equal vs Custom weights)
// ============================================================

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <eigen3/Eigen/Dense>
#include <iostream>
#include <map>
#include <numeric>
#include <stdexcept>
#include <string>
#include <vector>

#include "quantdream/legacy/csvReader/CsvReader.h"
#include "quantdream/legacy/monteCarlo/engine.h"
#include "quantdream/statistics/robust/center/winsorized_mean.h"
#include "quantdream/statistics/robust/robustEstimators.h"

// -----------------------------------------------------------
// Data structure
// -----------------------------------------------------------
using YFData = std::map<std::string,
                      std::map<std::string,
                      std::map<std::string, double>>>;

// -----------------------------------------------------------
// Helpers for cumulative returns
// -----------------------------------------------------------
static Eigen::VectorXd cumulative_compounded(const Eigen::VectorXd &ret) {
  const Eigen::Index T = ret.size();
  Eigen::VectorXd cum(T);
  if (T == 0) return cum;
  cum(0) = 1.0 + ret(0);
  for (Eigen::Index t = 1; t < T; ++t)
      cum(t) = cum(t - 1) * (1.0 + ret(t));
  return cum;
}

static Eigen::VectorXd cumulative_simple(const Eigen::VectorXd &ret) {
  const Eigen::Index T = ret.size();
  Eigen::VectorXd cum(T);
  if (T == 0) return cum;
  cum(0) = 1.0 + ret(0);
  for (Eigen::Index t = 1; t < T; ++t)
      cum(t) = 1.0 + ret.head(t + 1).sum();
  return cum;
}

// -----------------------------------------------------------
// Portfolio metrics helpers (risk-free Sharpe)
// -----------------------------------------------------------
struct Metrics {
  double mean_annual{};  // excess annualized mean (over rf)
  double vol_annual{};
  double sharpe{};
  double cagr{};         // from compounded path
  double var5{};         // based on daily returns
  double es5{};          // based on daily returns
};

#include <iomanip>

Metrics compute_metrics(const Eigen::VectorXd &ret,
                      const Eigen::VectorXd &cum,
                      double alpha = 0.05,
                      double risk_free_rate_annual = 0.02)
{
  Metrics m{};

  // Standard mean and volatility
  const double mean = ret.mean();
  const double vol  = std::sqrt((ret.array() - mean).square().mean());

  // Robust estimators from the QuantDream library
  const double trimmed_mean_val =
      qd::statistics::robust::center::trimmed_mean(ret, alpha);
  const double winsorized_mean_val =
      qd::statistics::robust::center::winsorized_mean(ret, alpha);

  // Daily risk-free rate conversion from annual rate
  const double r_f_daily =
      std::pow(1.0 + risk_free_rate_annual, 1.0 / 252.0) - 1.0;

  // Excess mean returns relative to risk-free rate
  const double excess_mean        = mean - r_f_daily;
  const double excess_trimmed     = trimmed_mean_val - r_f_daily;
  const double excess_winsorized  = winsorized_mean_val - r_f_daily;

  // Annualized metrics based on arithmetic mean
  m.mean_annual = excess_mean * 252.0;
  m.vol_annual  = vol * std::sqrt(252.0);
  m.sharpe      = (vol > 1e-9) ? (excess_mean / vol) * std::sqrt(252.0) : 0.0;

  // Robust Sharpe ratios using robust means
  const double sharpe_trimmed =
      (vol > 1e-9) ? (excess_trimmed / vol) * std::sqrt(252.0) : 0.0;
  const double sharpe_winsorized =
      (vol > 1e-9) ? (excess_winsorized / vol) * std::sqrt(252.0) : 0.0;

  // Compounded annual growth rate
  if (cum.size() > 1) {
      const double T_years = static_cast<double>(cum.size()) / 252.0;
      m.cagr = std::pow(cum(cum.size() - 1), 1.0 / T_years) - 1.0;
  }

  // Value at Risk and Expected Shortfall
  std::vector<double> v(ret.data(), ret.data() + ret.size());
  std::sort(v.begin(), v.end());
  size_t n = std::max<size_t>(1, static_cast<size_t>(alpha * v.size()));
  m.var5 = v[n - 1];
  m.es5 = std::accumulate(v.begin(), v.begin() + n, 0.0) / n;

  // Formatted output
  std::cout << std::fixed << std::setprecision(6);
  std::cout << "\n=== Portfolio Metrics ===\n";
  std::cout << std::left << std::setw(35) << "Mean (daily):"              << mean                << "\n";
  std::cout << std::left << std::setw(35) << "Trimmed mean (robust):"     << trimmed_mean_val    << "\n";
  std::cout << std::left << std::setw(35) << "Winsorized mean (robust):"  << winsorized_mean_val << "\n";
  std::cout << std::left << std::setw(35) << "Volatility (daily):"        << vol                 << "\n";
  std::cout << std::left << std::setw(35) << "Sharpe ratio (annualized):" << m.sharpe            << "\n";
  std::cout << std::left << std::setw(35) << "Trimmed Sharpe ratio:"      << sharpe_trimmed      << "\n";
  std::cout << std::left << std::setw(35) << "Winsorized Sharpe ratio:"   << sharpe_winsorized   << "\n";
  std::cout << std::left << std::setw(35) << "CAGR (annualized):"         << m.cagr              << "\n";
  std::cout << std::left << std::setw(35) << "Value at Risk (5%):"        << m.var5              << "\n";
  std::cout << std::left << std::setw(35) << "Expected Shortfall (5%):"   << m.es5               << "\n";

  return m;
}


// -----------------------------------------------------------
// Plotting function (full vs worst overlay)
// -----------------------------------------------------------
static void plot_full_vs_worst(const Eigen::VectorXd &meanFullEq,
                             const Eigen::VectorXd &upperFullEq,
                             const Eigen::VectorXd &lowerFullEq,
                             const Eigen::VectorXd &meanFullCu,
                             const Eigen::VectorXd &upperFullCu,
                             const Eigen::VectorXd &lowerFullCu,
                             const Eigen::VectorXd &meanWorstEq,
                             const Eigen::VectorXd &upperWorstEq,
                             const Eigen::VectorXd &lowerWorstEq,
                             const Eigen::VectorXd &meanWorstCu,
                             const Eigen::VectorXd &upperWorstCu,
                             const Eigen::VectorXd &lowerWorstCu,
                             const char* title,
                             double sigmaFactor,
                             double alpha,
                             bool compounded = true) {
  FILE* gp = popen("gnuplot -persistent", "w");
  if (!gp) throw std::runtime_error("Failed to open Gnuplot pipe.");

  if (compounded)
    fprintf(gp, "set ylabel 'Compounded cumulative return'\n");
  else
    fprintf(gp, "set ylabel 'Simple cumulative return'\n");

  fprintf(gp,
      "set terminal wxt size 1600,1000 enhanced font 'Arial,12'\n"
      "set title '%s'\n"
      "set xlabel 'Time step'\n"
      "set grid\n"
      "set key left top\n"
      "set style fill transparent solid 0.30 noborder\n"

      // Define clearer, distinct palette
      "set style line 1 lc rgb '#1f77b4' lw 2   # Equal full mean (blue)\n"
      "set style line 2 lc rgb '#aec7e8' lw 1   # Equal full band (light blue)\n"
      "set style line 3 lc rgb '#d62728' lw 2   # Custom full mean (red)\n"
      "set style line 4 lc rgb '#ff9896' lw 1   # Custom full band (light red)\n"

      // Darker desaturated tones for worst-case
      "set style line 5 lc rgb '#2c3e50' dt 2 lw 2   # Equal worst mean (dark blue)\n"
      "set style line 6 lc rgb '#7fb3d5' lw 1        # Equal worst band (steel blue)\n"
      "set style line 7 lc rgb '#78281f' dt 2 lw 2   # Custom worst mean (dark red)\n"
      "set style line 8 lc rgb '#e6b0aa' lw 1        # Custom worst band (pale red)\n"

      // Plot with better color separation
      "plot '-' with filledcurves ls 2 title 'Equal ±%.1fσ (full)', "
      "'-' with lines ls 1 title 'Equal Mean (full)', "
      "'-' with filledcurves ls 4 title 'Custom ±%.1fσ (full)', "
      "'-' with lines ls 3 title 'Custom Mean (full)', "
      "'-' with filledcurves ls 6 title 'Equal ±%.1fσ (worst %.0f%%%%)', "
      "'-' with lines ls 5 title 'Equal Mean (worst %.0f%%%%)', "
      "'-' with filledcurves ls 8 title 'Custom ±%.1fσ (worst %.0f%%%%)', "
      "'-' with lines ls 7 title 'Custom Mean (worst %.0f%%%%)'\n",
      title,
      sigmaFactor, sigmaFactor,
      sigmaFactor, alpha * 100,
      alpha * 100, sigmaFactor, alpha * 100,
      alpha * 100
  );

  auto send_band = [&](const Eigen::VectorXd &lower, const Eigen::VectorXd &upper) {
      for (int i = 0; i < lower.size(); ++i)
          fprintf(gp, "%d %f %f\n", i, lower(i), upper(i));
      fprintf(gp, "e\n");
  };
  auto send_line = [&](const Eigen::VectorXd &v) {
      for (int i = 0; i < v.size(); ++i)
          fprintf(gp, "%d %f\n", i, v(i));
      fprintf(gp, "e\n");
  };

  send_band(lowerFullEq, upperFullEq);
  send_line(meanFullEq);
  send_band(lowerFullCu, upperFullCu);
  send_line(meanFullCu);
  send_band(lowerWorstEq, upperWorstEq);
  send_line(meanWorstEq);
  send_band(lowerWorstCu, upperWorstCu);
  send_line(meanWorstCu);

  fflush(gp);
  pclose(gp);
}

// -----------------------------------------------------------
// Filter worst α% of paths
// -----------------------------------------------------------
std::vector<Eigen::VectorXd> filterWorstScenarios(
  const std::vector<Eigen::VectorXd>& paths, double alpha) {
  if (paths.empty()) return {};
  std::vector<std::pair<double, size_t>> finals;
  finals.reserve(paths.size());
  for (size_t i = 0; i < paths.size(); ++i)
      finals.emplace_back(paths[i](paths[i].size() - 1), i);
  std::sort(finals.begin(), finals.end(),
            [](auto &a, auto &b){ return a.first < b.first; });
  size_t nKeep = static_cast<size_t>(alpha * finals.size());
  nKeep = std::max<size_t>(1, nKeep);
  std::vector<Eigen::VectorXd> worst;
  worst.reserve(nKeep);
  for (size_t i = 0; i < nKeep; ++i)
      worst.push_back(paths[finals[i].second]);
  return worst;
}

// -----------------------------------------------------------
// Results container for paths and returns
// -----------------------------------------------------------
struct MCResult {
  Eigen::VectorXd mean;                   // mean cumulative path
  Eigen::VectorXd stddev;                 // stddev band for cumulative
  std::vector<Eigen::VectorXd> allPaths;  // all cumulative paths (for tail selection)
  std::vector<Eigen::VectorXd> allReturns;// all daily returns (for metrics)
};

// -----------------------------------------------------------
// Compute average path + store ALL returns for metrics
// -----------------------------------------------------------
MCResult compute_average_path_and_returns(MonteCarloEngine &mc,
                                        const std::vector<double> &weights,
                                        size_t nSim, size_t blockSize,
                                        bool compounded) {
  std::vector<Eigen::VectorXd> paths;
  paths.reserve(nSim);
  std::vector<Eigen::VectorXd> returns_all;
  returns_all.reserve(nSim);

  Eigen::Map<const Eigen::VectorXd> w(weights.data(), weights.size());

  for (size_t s = 0; s < nSim; ++s) {
      Eigen::MatrixXd sim = mc.runSingleSimulationVanilla(blockSize);
      if (sim.rows() == 0) continue;

      Eigen::VectorXd ret = sim * w;  // daily portfolio returns for THIS simulation
      returns_all.push_back(ret);

      Eigen::VectorXd cum = compounded ? cumulative_compounded(ret)
                                       : cumulative_simple(ret);
      paths.push_back(std::move(cum));
  }

  if (paths.empty()) throw std::runtime_error("No valid simulations.");

  const int T = paths[0].size();
  Eigen::VectorXd mean = Eigen::VectorXd::Zero(T);
  for (const auto &p : paths) mean += p;
  mean /= static_cast<double>(paths.size());

  Eigen::VectorXd stddev = Eigen::VectorXd::Zero(T);
  for (const auto &p : paths)
      stddev.array() += (p - mean).array().square();
  stddev = (stddev / (paths.size() - 1)).array().sqrt();

  return {mean, stddev, paths, returns_all};
}

// -----------------------------------------------------------
// Compute mean/stddev for worst α% cumulative paths
// -----------------------------------------------------------
MCResult compute_tail_average(const std::vector<Eigen::VectorXd>& allPaths, double alpha) {
  auto worst = filterWorstScenarios(allPaths, alpha);
  if (worst.empty()) throw std::runtime_error("No worst paths found.");

  const int T = worst[0].size();
  Eigen::VectorXd mean = Eigen::VectorXd::Zero(T);
  for (const auto &p : worst) mean += p;
  mean /= static_cast<double>(worst.size());

  Eigen::VectorXd stddev = Eigen::VectorXd::Zero(T);
  for (const auto &p : worst)
      stddev.array() += (p - mean).array().square();
  stddev = (stddev / (worst.size() - 1)).array().sqrt();

  return {mean, stddev, worst, {}}; // returns not used here
}

// -----------------------------------------------------------
// MAIN
// -----------------------------------------------------------
int main() {
  try {
      const std::string FILENAME = "../standalone/datasets/msci_portfolio.csv";
      YFData data = getYFCSV(FILENAME);

      // --- Parameters ---
      const size_t nSim = 10000;
      const size_t nSamples = 252 * 5;   // horizon length controlled by engine
      const size_t blockSize = 5;
      const double alpha = 0.05;         // worst 5%
      const double sigmaFactor = 2.0;    // band width for plots
      const double riskFreeRate = 0.02;  // 2% annual RF

      MonteCarloEngine mc(data, nSim, nSamples, blockSize, 5);
      mc.selectCategory("Close");

      const size_t nAssets = 6;
      std::vector<double> w_equal(nAssets, 1.0 / nAssets);
      std::vector<double> w_custom = {0.12, 0.10, 0.28, 0.27, 0.11, 0.12};

      // === Run simulations (COMP and SIMPLE), storing cumulative paths AND returns ===
      MCResult eqComp    = compute_average_path_and_returns(mc, w_equal, nSim, blockSize, true);
      MCResult cuComp    = compute_average_path_and_returns(mc, w_custom, nSim, blockSize, true);
      MCResult eqSimple  = compute_average_path_and_returns(mc, w_equal, nSim, blockSize, false);
      MCResult cuSimple  = compute_average_path_and_returns(mc, w_custom, nSim, blockSize, false);

      // === Worst α% by cumulative final value ===
      MCResult eqCompWorst    = compute_tail_average(eqComp.allPaths, alpha);
      MCResult cuCompWorst    = compute_tail_average(cuComp.allPaths, alpha);
      MCResult eqSimpleWorst  = compute_tail_average(eqSimple.allPaths, alpha);
      MCResult cuSimpleWorst  = compute_tail_average(cuSimple.allPaths, alpha);

      // === Flatten ensemble returns for metrics ===
      auto flatten = [](const std::vector<Eigen::VectorXd>& vecs) {
          size_t total = 0;
          for (const auto& v : vecs) total += static_cast<size_t>(v.size());
          std::vector<double> tmp; tmp.reserve(total);
          for (const auto& v : vecs) tmp.insert(tmp.end(), v.data(), v.data() + v.size());
          return Eigen::Map<Eigen::VectorXd>(tmp.data(), static_cast<Eigen::Index>(tmp.size())).eval();
      };

      Eigen::VectorXd ret_eq_all = flatten(eqComp.allReturns);
      Eigen::VectorXd ret_cu_all = flatten(cuComp.allReturns);

      // === Metrics from ENSEMBLE daily returns; CAGR from compounded mean path ===
      Metrics m_eq = compute_metrics(ret_eq_all, eqComp.mean, alpha, riskFreeRate);
      Metrics m_cu = compute_metrics(ret_cu_all, cuComp.mean, alpha, riskFreeRate);

      // === Clean reporting utility ===
      auto report = [&](const std::string& label,
                        const Metrics& m,
                        const Eigen::VectorXd& ret,
                        double rf) {
          using namespace qd::statistics::robust::center;

          double trimmed = trimmed_mean(ret, 0.05);
          double winsor  = winsorized_mean(ret, 0.05);
          double vol     = std::sqrt((ret.array() - ret.mean()).square().mean());
          double rf_daily = std::pow(1.0 + rf, 1.0 / 252.0) - 1.0;

          double excess_trimmed = trimmed - rf_daily;
          double excess_winsor  = winsor  - rf_daily;

          double sharpe_trimmed =
              (vol > 1e-9) ? (excess_trimmed / vol) * std::sqrt(252.0) : 0.0;
          double sharpe_winsor  =
              (vol > 1e-9) ? (excess_winsor  / vol) * std::sqrt(252.0) : 0.0;

          std::cout << std::fixed << std::setprecision(6);

          // Section title
          std::cout << "\n" << std::string(80, '=') << "\n";
          std::cout << "  " << label << " Portfolio Performance (Annualized Statistics)\n";
          std::cout << std::string(80, '=') << "\n";

          // Standard annualized metrics
          std::cout << std::left << std::setw(30) << "Excess Mean (annualized)" << ": " << std::setw(12) << m.mean_annual
                    << std::left << std::setw(18) << "Volatility"               << ": " << std::setw(12) << m.vol_annual
                    << std::left << std::setw(12) << "Sharpe"                   << ": " << m.sharpe << "\n";

          std::cout << std::left << std::setw(30) << "CAGR (annualized)"        << ": " << std::setw(12) << m.cagr
                    << std::left << std::setw(18) << "VaR(5%)"                  << ": " << std::setw(12) << m.var5
                    << std::left << std::setw(12) << "ES(5%)"                   << ": " << m.es5 << "\n";

          // Robust section
          std::cout << "\n" << std::string(80, '-') << "\n";
          std::cout << "  Robust Sharpe Ratios (α = 5%)\n";
          std::cout << std::string(80, '-') << "\n";
          std::cout << std::left << std::setw(30) << "Trimmed Mean (robust)"    << ": " << std::setw(12) << trimmed
                    << std::left << std::setw(18) << "Trimmed Sharpe"           << ": " << sharpe_trimmed << "\n";
          std::cout << std::left << std::setw(30) << "Winsorized Mean (robust)" << ": " << std::setw(12) << winsor
                    << std::left << std::setw(18) << "Winsorized Sharpe"        << ": " << sharpe_winsor << "\n";
      };

      // === Print clean, organized ensemble summaries ===
      std::cout << "\n\n=== Annualized Portfolio Statistics ===\n";
      report("Equal-weighted",  m_eq, ret_eq_all, riskFreeRate);
      report("Custom-weighted", m_cu, ret_cu_all, riskFreeRate);
      std::cout << "\nRisk-free rate (annual): " << std::fixed << std::setprecision(2)
                << riskFreeRate * 100.0 << "%\n";


      // === Plot: compounded cumulative returns (Full vs Worst) ===
      plot_full_vs_worst(eqComp.mean,   eqComp.mean   + sigmaFactor * eqComp.stddev,   eqComp.mean   - sigmaFactor * eqComp.stddev,
                         cuComp.mean,   cuComp.mean   + sigmaFactor * cuComp.stddev,   cuComp.mean   - sigmaFactor * cuComp.stddev,
                         eqCompWorst.mean, eqCompWorst.mean + sigmaFactor * eqCompWorst.stddev, eqCompWorst.mean - sigmaFactor * eqCompWorst.stddev,
                         cuCompWorst.mean, cuCompWorst.mean + sigmaFactor * cuCompWorst.stddev, cuCompWorst.mean - sigmaFactor * cuCompWorst.stddev,
                         "Equal vs Custom (Full vs Worst 5%) [Compounded]",
                         sigmaFactor, alpha, true);

      // === Plot: simple cumulative returns (Full vs Worst) ===
      plot_full_vs_worst(eqSimple.mean,   eqSimple.mean   + sigmaFactor * eqSimple.stddev,   eqSimple.mean   - sigmaFactor * eqSimple.stddev,
                         cuSimple.mean,   cuSimple.mean   + sigmaFactor * cuSimple.stddev,   cuSimple.mean   - sigmaFactor * cuSimple.stddev,
                         eqSimpleWorst.mean, eqSimpleWorst.mean + sigmaFactor * eqSimpleWorst.stddev, eqSimpleWorst.mean - sigmaFactor * eqSimpleWorst.stddev,
                         cuSimpleWorst.mean, cuSimpleWorst.mean + sigmaFactor * cuSimpleWorst.stddev, cuSimpleWorst.mean - sigmaFactor * cuSimpleWorst.stddev,
                         "Equal vs Custom (Full vs Worst 5%) [Simple]",
                         sigmaFactor, alpha, false);


  } catch (const std::exception &e) {
      std::cerr << "Error: " << e.what() << std::endl;
      return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}
