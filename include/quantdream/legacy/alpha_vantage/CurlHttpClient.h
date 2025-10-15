// Created by Giuseppe Priolo on 21/09/25.
//

#ifndef CURLHTTPCLIENT_H
#define CURLHTTPCLIENT_H
#include "IHttpClient.h"

#include <string>

class CurlHttpClient : public IHttpClient {
public:
    CurlHttpClient();
    ~CurlHttpClient() override;

    std::string get(const std::string& url) override;
};

#endif //CURLHTTPCLIENT_H
