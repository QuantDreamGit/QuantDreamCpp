//
// Created by Giuseppe Priolo on 21/09/25.
//

#include "quantdream/legacy/alpha_vantage/Parser.h"
#include "external/rapidjson/document.h"
#include "external/rapidjson/error/en.h"

using namespace AlphaVantage;

void Parser::parseJsonResponse(const std::string &jsonResponse,
                                  const std::string &symbol,
                                  TimeSeries &timeSeries)
    {
        rapidjson::Document document;
        if (document.Parse(jsonResponse.c_str()).HasParseError()) {
            throw std::runtime_error(
                "JSON parse error: " +
                std::string(rapidjson::GetParseError_En(document.GetParseError())) +
                " at offset " + std::to_string(document.GetErrorOffset())
            );
        }

        // Check for API error messages
        if (document.HasMember("Error Message")) {
            throw std::runtime_error("API Error: " + std::string(document["Error Message"].GetString()));
        }

        // Identify which time series is present
        const char* possibleKeys[] = {
            "Time Series (Daily)",
            "Weekly Time Series",
            "Monthly Time Series",
            "Monthly Adjusted Time Series"
        };

        const rapidjson::Value* timeSeriesNode = nullptr;
        for (const auto& key : possibleKeys) {
            if (document.HasMember(key)) {
                timeSeriesNode = &document[key];
                break;
            }
        }

        if (!timeSeriesNode) {
            throw std::runtime_error("Unexpected JSON structure: no recognized time series key found");
        }

        // Iterate over all date entries
        for (auto it = timeSeriesNode->MemberBegin(); it != timeSeriesNode->MemberEnd(); ++it) {
            const std::string ts = it->name.GetString();
            const auto& entry = it->value;

            // Mandatory OHLCV fields
            double o = entry.HasMember("1. open")   ? std::stod(entry["1. open"].GetString())   : 0.0;
            double h = entry.HasMember("2. high")   ? std::stod(entry["2. high"].GetString())   : 0.0;
            double l = entry.HasMember("3. low")    ? std::stod(entry["3. low"].GetString())    : 0.0;
            double c = entry.HasMember("4. close")  ? std::stod(entry["4. close"].GetString())  : 0.0;
            double v = 0.0;

            // Volume can be under "5. volume" or "6. volume" depending on endpoint
            if (entry.HasMember("5. volume"))
                v = std::stod(entry["5. volume"].GetString());
            else if (entry.HasMember("6. volume"))
                v = std::stod(entry["6. volume"].GetString());

            // Create a new OHLCV data point
            auto point = std::make_shared<OHLCVDataPoint>(ts, o, h, l, c, v);

            // Insert into TimeSeries for this symbol
            timeSeries.addDataPoint(symbol, point);
        }
    }
