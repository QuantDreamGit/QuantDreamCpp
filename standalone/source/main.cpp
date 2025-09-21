#include <iostream>
#include "alpha_vantage/AlphaVantage.h"
#include "alpha_vantage/CurlHttpClient.h"
#include <memory>
#include <string>

int main() {
    const std::string API = "WFE9OEXRRTBBUP7P";
    const std::string SYMBOL = "IBM";

    // Create an instance of the CurlHttpClient
    auto httpClient = std::make_shared<CurlHttpClient>();

    // Alpha Vantage Client initialization
    AlphaVantage::Client client(API, httpClient);

    // Fetch daily time series data for a specific stock symbol

    try {
        std::string data = client.fetchDailyTimeSeries(SYMBOL);
        std::cout << "Data for " << SYMBOL << ":\n" << data << std::endl;
    } catch (const std::exception &e) {
        std::cerr << "Error fetching data: " << e.what() << std::endl;
    }

    return 0;
}
