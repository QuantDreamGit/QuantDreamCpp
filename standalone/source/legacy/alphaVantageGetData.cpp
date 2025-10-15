#include <iostream>
#include "quantdream/legacy/alpha_vantage/AlphaVantage.h"
#include "quantdream/legacy/alpha_vantage/CurlHttpClient.h"
#include <memory>
#include <string>

#include "quantdream/legacy/alpha_vantage/Parser.h"

int main() {
    const std::string API = "Z66IST5JZ5NNWHJ2";// "WFE9OEXRRTBBUP7P";
    // List of Symbols
    const std::vector<std::string> SYMBOLS = {
        "IBM"
    };
    // Initialize TimeSeries object
    TimeSeries ts;

    // Create an instance of the CurlHttpClient
    auto httpClient = std::make_shared<CurlHttpClient>();

    // Alpha Vantage Client initialization
    AlphaVantage::Client client(API, httpClient);

    try {
        for (const std::string& symbol : SYMBOLS) {
            std::string rawJson = client.fetchDailyTimeSeries(symbol);
            if (rawJson.empty()) {
                std::cerr << "Failed to fetch data for symbol: " << symbol << std::endl;
                continue;
            }
            AlphaVantage::Parser::parseJsonResponse(rawJson, symbol, ts);
        }

        for (const auto& dp : ts.getDataPoints("IBM")) {
            auto* ohlc = dynamic_cast<OHLCVDataPoint*>(dp.get());
            if (ohlc) {
                std::cout << ohlc->getTimestamp()
                          << " O:" << ohlc->getOpen()
                          << " H:" << ohlc->getHigh()
                          << " L:" << ohlc->getLow()
                          << " C:" << ohlc->getClose()
                          << " V:" << ohlc->getVolume()
                          << "\n";
            }
        }

    } catch (const std::exception &e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
}
