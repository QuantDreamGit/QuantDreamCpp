//
// Created by Giuseppe Priolo on 18/09/25.
//

#ifndef CLIENTS_H
#define CLIENTS_H

#include <string>

class IHttpClient {
public:
    virtual ~IHttpClient() = default;
    virtual std::string get(const std::string& url) = 0;
};

#endif //CLIENTS_H
