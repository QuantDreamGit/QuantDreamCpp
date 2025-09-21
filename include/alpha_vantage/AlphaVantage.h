//
// Created by Giuseppe Priolo on 21/09/25.
//

#ifndef ALPHAVANTAGE_H
#define ALPHAVANTAGE_H
#include <string>
#include "IHttpClient.h"


namespace AlphaVantage {
    class Client {
    public:
        Client(const std::string& apiKey, std::shared_ptr<IHttpClient> httpClient);
        ~Client();

        std::string fetchDailyTimeSeries(const std::string& symbol);
     private:
        std::string _apiKey;
        std::shared_ptr<IHttpClient> _httpClient;
    };
}

#endif //ALPHAVANTAGE_H
