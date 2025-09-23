//
// Created by user on 9/23/25.
//

#ifndef QUANTDREAMCPP_CSVREADER_H
#define QUANTDREAMCPP_CSVREADER_H

#include <map>
#include <vector>
#include <string>

// YFinance Data Structure
using YFData = std::map<std::string,                      // Date
                        std::map<std::string,             // Category
                        std::map<std::string,             // Ticker
                        double>>>;                        // Value

// Function to split a CSV line into tokens
std::vector<std::string> splitCSVLine(const std::string &line);

// Function to read YFinance CSV file and return structured data
YFData getYFCSV(const std::string &filename);
#endif  // QUANTDREAMCPP_CSVREADER_H
