#include <iostream>
#include <string>
#include <nlohmann/json.hpp>

namespace pypyweb
{
    void init();
    size_t writeCallbackFunction( char* ptr, size_t size, size_t nmemb, void* userdata);
    bool httpGet(std::string url, std::string& responseData);
    bool httpGetJson(std::string url, nlohmann::json& resOut);
    void cleanup();
}

