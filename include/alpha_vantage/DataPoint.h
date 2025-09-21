//
// Created by Giuseppe Priolo on 21/09/25.
//

#ifndef DATAPOINT_H
#define DATAPOINT_H
#include "AlphaVantage.h"
#include <string>

class DataPoint {
public:
    virtual ~DataPoint() = default;
    virtual const std::string& getTimestamp() const = 0;
    virtual double getOpen() const = 0;
    virtual double getHigh() const = 0;
    virtual double getLow() const = 0;
    virtual double getClose() const = 0;
    virtual double getVolume() const = 0;
};

class OHLCVDataPoint : public DataPoint {
private:
    std::string timestamp;
    double open;
    double high;
    double low;
    double close;
    double volume;

public:
    OHLCVDataPoint(const std::string &ts,
                   double o,
                   double h,
                   double l,
                   double c,
                   double v)
        : timestamp(ts), open(o), high(h), low(l), close(c), volume(v) {}

    // --- Getters ---
    const std::string& getTimestamp() const { return timestamp; }
    double getOpen() const { return open; }
    double getHigh() const { return high; }
    double getLow() const { return low; }
    double getClose() const { return close; }
    double getVolume() const { return volume; }
};


#endif //DATAPOINT_H
