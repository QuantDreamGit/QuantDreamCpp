//
// Created by Giuseppe Priolo on 21/09/25.
//

#include "alpha_vantage/AlphaVantage.h"
#include "alpha_vantage/TimeSeries.h"

#include <stdexcept>
#include <string>

using namespace AlphaVantage;

namespace AlphaVantage {
    // AlphaVantage Client Implementation
    Client::Client(const std::string &apiKey, std::shared_ptr<IHttpClient> httpClient)
        : _apiKey(std::move(apiKey)),
          _httpClient(std::move(httpClient))
        {};

    // Destructor
    Client::~Client() = default;

    // Fetch Raw Time Series Data
    std::string Client::fetchDailyTimeSeries(const std::string &symbol) const {
        std::string url = "https://www.alphavantage.co/query?function=TIME_SERIES_DAILY"
                          "&symbol=" + symbol +
                          "&apikey=" + _apiKey;
        return _httpClient->get(url);
    }

    std::string Client::fetchWeeklyTimeSeries(const std::string &symbol) const {
        std::string url = "https://www.alphavantage.co/query?function=TIME_SERIES_WEEKLY"
                          "&symbol=" + symbol +
                          "&apikey=" + _apiKey;
        return _httpClient->get(url);
    }

    std::string Client::fetchMonthlyTimeSeries(const std::string &symbol) const {
        std::string url = "https://www.alphavantage.co/query?function=TIME_SERIES_MONTHLY"
                          "&symbol=" + symbol +
                          "&apikey=" + _apiKey;
        return _httpClient->get(url);
    }
}