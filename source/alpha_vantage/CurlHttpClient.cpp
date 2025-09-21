//
// Created by Giuseppe Priolo on 21/09/25.
//

#include "alpha_vantage/CurlHttpClient.h"

#include <curl/curl.h>
#include <stdexcept>
#include <string>

namespace {
    size_t writeCallback(void *contents, size_t size, size_t nmemb, void *stream) {
        // Append the received data to the string
        // 'stream' is a pointer to the std::string object
        // 'contents' is the data received
        static_cast<std::string*>(stream)->append(static_cast<char*>(contents), size * nmemb);
        return size * nmemb;
    }
}

CurlHttpClient::CurlHttpClient() {
    // Initialize libcurl using standard initialization
    curl_global_init(CURL_GLOBAL_DEFAULT);
}

CurlHttpClient::~CurlHttpClient() {
    // Cleanup libcurl resources
    curl_global_cleanup();
}

std::string CurlHttpClient::get(const std::string& url) {
    // Buffer variable to store the response data
    std::string responseBuffer;

    // Initialize a CURL handle
    CURL* curl = curl_easy_init();
    if (!curl) {
        throw std::runtime_error("Failed to initialize CURL");
    }

    // Set CURL options
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &responseBuffer);

    // Perform the request
    CURLcode res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
        // Cleanup CURL handle before throwing an error
        curl_easy_cleanup(curl);
        throw std::runtime_error(curl_easy_strerror(res));
    }

    // Cleanup CURL handle
    curl_easy_cleanup(curl);

    return responseBuffer;
}
