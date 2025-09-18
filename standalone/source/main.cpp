#include <iostream>
#include <string>
#include <curl/curl.h>
#include "external/rapidjson/document.h"
#include "external/rapidjson/error/en.h"

using namespace rapidjson;

// Callback for libcurl
size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

int main() {
    std::string apiKey = "WFE9OEXRRTBBUP7P";
    std::string symbol = "IBM";
    std::string url = "https://www.alphavantage.co/query?function=TIME_SERIES_DAILY&symbol="
                      + symbol + "&apikey=" + apiKey;

    CURL* curl;
    CURLcode res;
    std::string readBuffer;

    curl_global_init(CURL_GLOBAL_DEFAULT);
    curl = curl_easy_init();
    if(curl) {
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);

        res = curl_easy_perform(curl);
        if(res != CURLE_OK) {
            std::cerr << "curl_easy_perform() failed: "
                      << curl_easy_strerror(res) << std::endl;
        }
        curl_easy_cleanup(curl);
    }
    curl_global_cleanup();

    // Parse JSON with RapidJSON
    Document doc;
    ParseResult parseResult = doc.Parse(readBuffer.c_str());
    if (!parseResult) {
        std::cerr << "JSON parse error: "
                  << GetParseError_En(parseResult.Code())
                  << " (offset " << parseResult.Offset() << ")"
                  << std::endl;
        return 1;
    }

    if (doc.HasMember("Time Series (Daily)")) {
        const Value& timeSeries = doc["Time Series (Daily)"];

        // Get first (latest) entry
        if (timeSeries.MemberBegin() != timeSeries.MemberEnd()) {
            auto itr = timeSeries.MemberBegin();
            std::string latestDate = itr->name.GetString();
            std::string closePrice = itr->value["4. close"].GetString();

            std::cout << "Latest date: " << latestDate << std::endl;
            std::cout << "Close price: " << closePrice << std::endl;
        }
    } else {
        std::cerr << "Time Series (Daily) not found in response." << std::endl;
    }

    return 0;
}
