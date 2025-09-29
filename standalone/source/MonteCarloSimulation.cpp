//
// Created by user on 9/24/25.
//
// Description: Example of loading a CSV file with YFinance data structure
// It loads the data and prints the last n rows
#include <iostream>
#include <ostream>
#include <string>

#include "csvReader/CsvReader.h"
#include "monteCarlo/engine.h"
#include "monteCarlo/riskMeasures.h"

// YFinance Data Structure
using YFData = std::map<std::string,                      // Date
                        std::map<std::string,             // Category
                        std::map<std::string,             // Ticker
                        double>>>;                        // Value

int main() {
  // Load CSV file
  const std::string FILENAME = "../standalone/datasets/msci_portfolio.csv";
  YFData data = getYFCSV(FILENAME);

  // Run Monte Carlo Simulation
  MonteCarloEngine mc(data, 1, 10000, 5);
  mc.selectCategory("Close");
  mc.runSimulation(RiskMeasure::VaR);



}