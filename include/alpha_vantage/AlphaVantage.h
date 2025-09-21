//
// Created by Giuseppe Priolo on 21/09/25.
//

#ifndef ALPHAVANTAGE_H
#define ALPHAVANTAGE_H
#include <string>
#include "IHttpClient.h"
#include "TimeSeries.h"

namespace AlphaVantage {
    class Client {
    public:
        Client(const std::string& apiKey, std::shared_ptr<IHttpClient> httpClient);
        ~Client();

        // Fetch raw JSON data (no parsing)
        std::string fetchDailyTimeSeries(const std::string& symbol) const;
        std::string fetchWeeklyTimeSeries(const std::string& symbol) const;
        std::string fetchMonthlyTimeSeries(const std::string& symbol) const;
     private:
        std::string _apiKey;
        std::shared_ptr<IHttpClient> _httpClient;
    };
}

#endif //ALPHAVANTAGE_H
