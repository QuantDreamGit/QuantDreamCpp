//
// Created by user on 9/23/25.
//
#include <vector>
#include <map>
#include <string>
#include <sstream>
#include <fstream>
#include <limits>

// YFinance Data Structure
using YFData = std::map<std::string,                      // Date
                        std::map<std::string,             // Category
                        std::map<std::string,             // Ticker
                        double>>>;                        // Value

std::vector<std::string> splitCSVLine(const std::string &line) {
  std::vector<std::string> result;
  // Treat the line as a stream so that we can use std::getline
  std::stringstream ss(line);
  std::string token;

  // Split the line by commas and store each token in the result vector
  while (std::getline(ss, token, ',')) {
    result.push_back(token);
  }

  return result;
}

YFData getYFCSV(const std::string &fileName) {
  std::ifstream file(fileName);
  std::string line;
  YFData data;

  // Read the categories' line
  std::getline(file, line);
  std::vector<std::string> categories = splitCSVLine(line);

  // Read the tickers' line
  std::getline(file, line);
  std::vector<std::string> tickers = splitCSVLine(line);

  // Skip the Date,,,,,, line
  std::getline(file, line);

  while (std::getline(file, line)) {
    std::vector<std::string> row = splitCSVLine(line);
    // Skip empty lines or lines with empty date
    if  (row.empty() || row[0].empty()) continue;

    // The first column is the date
    std::string date = row[0];

    // Iterate over the rest of the columns
    for (size_t i = 1; i < row.size(); i++) {
      // Skip empty cells
      if (row[i].empty()) {
        // Handle "" values by storing NaN
        data[date][categories[i]][tickers[i]] = std::numeric_limits<double>::quiet_NaN();
      } else {
        // Convert the value to double and store it in the data structure
        data[date][categories[i]][tickers[i]] = std::stod(row[i]);
      }
    }
  }
  return data;
}