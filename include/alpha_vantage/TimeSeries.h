//
// Created by Giuseppe Priolo on 21/09/25.
//

#ifndef TIMESERIES_H
#define TIMESERIES_H

#include "DataPoint.h"
#include <map>
#include <vector>

class TimeSeries {
    // Symbol --> DataPoints
    std::map<std::string, std::vector<std::shared_ptr<DataPoint>>> data;

public:
    void addDataPoint(const std::string &symbol, const std::shared_ptr<DataPoint>& point) {
        /* If the symbol does not exist, a new entry is created.
         * The vector of DataPoint is initialized as empty.
         * Then, the new DataPoint is added to the vector.
         * If the symbol already exists, the new DataPoint is simply added to the existing vector.
         */
        data[symbol].push_back(point);
    }

    const std::vector<std::shared_ptr<DataPoint>>& getDataPoints(const std::string &symbol) const {
        /*
         * Returns a reference to the vector of DataPoint for the given symbol.
         * If the symbol does not exist, returns an empty vector.
         */
        static const std::vector<std::shared_ptr<DataPoint>> empty;
        auto it = data.find(symbol);
        return it != data.end() ? it->second : empty;
    }

    const std::map<std::string, std::vector<std::shared_ptr<DataPoint>>>& getAllData() const {
        return data;
    }
};

#endif //TIMESERIES_H
