//
// Created by Giuseppe Priolo on 21/09/25.
//

#ifndef PARSER_H
#define PARSER_H
#include <string>
#include "quantdream/legacy/alpha_vantage/TimeSeries.h"

namespace AlphaVantage {
    class Parser {
    public:
        static void parseJsonResponse(const std::string &jsonResponse,
                                      const std::string &symbol,
                                      TimeSeries &timeSeries);
    };
}
#endif //PARSER_H
