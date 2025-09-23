// Description: Example of loading a CSV file with YFinance data structure
// It loads the data and prints the last n rows
#include <iostream>
#include <ostream>
#include <string>

#include "csvReader/CsvReader.h"

// YFinance Data Structure
using YFData = std::map<std::string,                      // Date
                        std::map<std::string,             // Category
                        std::map<std::string,             // Ticker
                        double>>>;                        // Value

int main() {
  size_t n = 1;
  const std::string filename = "../standalone/datasets/etf_aggressive_portfolio.csv";
  YFData data = getYFCSV(filename);
  const size_t dataSize = data.size();

  // Check if there is enough data, otherwise print the entire data
  if (dataSize < n) {
    n = dataSize;
  }

  // get the starting index date
  const size_t initialIndex = dataSize - n;

  // Print the loaded data
  size_t rowIndex = 0;
  for (const auto& [date, categories] : data) {
    if (rowIndex++ < initialIndex) continue; // Skip until the initial index
    std::cout << "Date: " << date << "\n";
    for (const auto& [category, tickers] : categories) {
      std::cout << "  Category: " << category << "\n";
      for (const auto& [ticker, value] : tickers) {
        std::cout << "    Ticker: " << ticker << ", Value: " << value << "\n";
      }
    }
  }
}