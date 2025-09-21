//
// Created by Giuseppe Priolo on 21/09/25.
//

#include "alpha_vantage/AlphaVantage.h"

#include <curl/curl.h>
#include <stdexcept>
#include <string>
#include "external/rapidjson/document.h"
#include "external/rapidjson/error/en.h"

using namespace AlphaVantage;

namespace AlphaVantage {
    // AlphaVantage Client Implementation
    Client::Client(const std::string &apiKey, std::shared_ptr<IHttpClient> httpClient)
        : _apiKey(std::move(apiKey)), _httpClient(std::move(httpClient)) {};

    // Destructor
    Client::~Client() = default;

    // Fetch Daily Time Series Data
    std::string Client::fetchDailyTimeSeries(const std::string &symbol) {
        std::string url = "https://www.alphavantage.co/query?function=TIME_SERIES_DAILY"
                          "&symbol=" + symbol +
                          "&apikey=" + _apiKey;

        return _httpClient->get(url);
    }
}