//
// Created by user on 9/26/25.
//

#ifndef QUANTDREAMCPP_RISKMEASURES_H
#define QUANTDREAMCPP_RISKMEASURES_H
#include <vector>

// Generic function will be used to avoid code duplication for the different risk measures
// Define possible risk measures as enum
enum class RiskMeasure {
  VaR,
  ES
};
template <typename T>
std::vector<double> computeRisk(const T &returns, const RiskMeasure measure, const double &alpha);



#endif  // QUANTDREAMCPP_RISKMEASURES_H
