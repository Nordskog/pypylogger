#include <iostream>
#include <string>
#include <curl/curl.h>
#include <nlohmann/json.hpp>
#include <obs-module.h>

#include "pypy_web.hpp"

using namespace std;

CURL *curl = nullptr;

void pypyweb::init()
{
    if ( curl == nullptr )
    {
        curl = curl_easy_init();
    }
   
}

size_t pypyweb::writeCallbackFunction( char* ptr, size_t size, size_t nmemb, void* userdata)
{
    string* response_data = static_cast<string*>(userdata);
    response_data->append(ptr, size * nmemb);
    return size * nmemb;
}

bool pypyweb::httpGet(string url, string& responseData)
{
    init();

    curl_easy_reset(curl);
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "libcurl-agent/1.0");

    // Can't use lambdas without converting to c function pointer, even though it will compile.
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, pypyweb::writeCallbackFunction);

    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &responseData);

    CURLcode res = curl_easy_perform(curl);



    if (res != CURLE_OK) 
    {
        blog( LOG_INFO, "Failed to perform HTTP request: %s", curl_easy_strerror(res));	
        return false;
    }

    long http_code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
    if (http_code != 200) 
    {
        blog( LOG_INFO, "Unexpected HTTP response code: %i", http_code);	
        return false;
    }

    return true;
}

bool pypyweb::httpGetJson(string url, nlohmann::json& resOut)
{
    string responseData = "";
    if (!httpGet(url, responseData))
    {
        return false;
    }

    char* contentTypePointer;
    curl_easy_getinfo(curl, CURLINFO_CONTENT_TYPE, &contentTypePointer);
    if (contentTypePointer == nullptr)
    {
        blog( LOG_INFO, "Response had no content type.");
        return false;
    }

    string contentType = contentTypePointer;

    if ( contentType.find("application/json") != std::string::npos)
    {
        if (responseData.length() <= 0)
        {
            blog( LOG_INFO, "Response body was empty");
            return false;
        }

        resOut = nlohmann::json::parse(responseData);

        return true;
    }
    else 
    {
        blog( LOG_INFO, "Unexpected content type: %s", contentType.c_str());

        return false;
    }
}

void pypyweb::cleanup()
{
    if (curl != nullptr)
    {
         curl_easy_cleanup(curl);
         curl = nullptr;
    }
}