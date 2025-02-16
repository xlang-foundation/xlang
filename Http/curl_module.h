/*
Copyright (C) 2024 The XLang Foundation
Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0
*/

#pragma once

#include "xpackage.h"
#include "xlang.h"
#include <string>
#include <curl/curl.h>

namespace X
{
    class Curl
    {
        // Underlying libcurl easy handle.
        CURL* m_curl;
        // Buffer for the response body.
        std::string m_response;
        // Error string.
        std::string m_error;
        // Libcurl error buffer.
        char m_errorBuffer[CURL_ERROR_SIZE];
        // For storing custom HTTP headers.
        curl_slist* m_headerList;
    public:
        BEGIN_PACKAGE(Curl)
            // API functions:
            APISET().AddFunc<2>("setOpt", &Curl::SetOpt);
        APISET().AddFunc<0>("perform", &Curl::Perform);
        APISET().AddFunc<1>("getInfo", &Curl::GetInfo);
        APISET().AddFunc<0>("reset", &Curl::Reset);
        APISET().AddFunc<1>("escape", &Curl::Escape);
        APISET().AddFunc<1>("unescape", &Curl::Unescape);
        // Properties:
        APISET().AddProp("response", &Curl::GetResponse);
        APISET().AddProp("error", &Curl::GetError);
        // Exported constants (selected libcurl options and info keys):
        APISET().AddConst("WRITEDATA", CURLOPT_WRITEDATA);
        APISET().AddConst("URL", CURLOPT_URL);
        APISET().AddConst("PORT", CURLOPT_PORT);
        APISET().AddConst("POST", CURLOPT_POST);
        APISET().AddConst("POSTFIELDS", CURLOPT_POSTFIELDS);
        APISET().AddConst("HTTPHEADER", CURLOPT_HTTPHEADER);
        APISET().AddConst("FOLLOWLOCATION", CURLOPT_FOLLOWLOCATION);
        APISET().AddConst("TIMEOUT", CURLOPT_TIMEOUT);
        APISET().AddConst("USERAGENT", CURLOPT_USERAGENT);
        APISET().AddConst("VERBOSE", CURLOPT_VERBOSE);
        APISET().AddConst("CUSTOMREQUEST", CURLOPT_CUSTOMREQUEST);
        APISET().AddConst("PROXY", CURLOPT_PROXY);
        APISET().AddConst("PROXYPORT", CURLOPT_PROXYPORT);
        APISET().AddConst("COOKIE", CURLOPT_COOKIE);
        APISET().AddConst("WRITEFUNCTION", CURLOPT_WRITEFUNCTION);

        APISET().AddConst("CURLOPT_WRITEDATA", CURLOPT_WRITEDATA);
        APISET().AddConst("CURLOPT_URL", CURLOPT_URL);
        APISET().AddConst("CURLOPT_PORT", CURLOPT_PORT);
        APISET().AddConst("CURLOPT_POST", CURLOPT_POST);
        APISET().AddConst("CURLOPT_POSTFIELDS", CURLOPT_POSTFIELDS);
        APISET().AddConst("CURLOPT_HTTPHEADER", CURLOPT_HTTPHEADER);
        APISET().AddConst("CURLOPT_FOLLOWLOCATION", CURLOPT_FOLLOWLOCATION);
        APISET().AddConst("CURLOPT_TIMEOUT", CURLOPT_TIMEOUT);
        APISET().AddConst("CURLOPT_USERAGENT", CURLOPT_USERAGENT);
        APISET().AddConst("CURLOPT_VERBOSE", CURLOPT_VERBOSE);
        APISET().AddConst("CURLOPT_CUSTOMREQUEST", CURLOPT_CUSTOMREQUEST);
        APISET().AddConst("CURLOPT_PROXY", CURLOPT_PROXY);
        APISET().AddConst("CURLOPT_PROXYPORT", CURLOPT_PROXYPORT);
        APISET().AddConst("CURLOPT_COOKIE", CURLOPT_COOKIE);
        APISET().AddConst("CURLOPT_WRITEFUNCTION", CURLOPT_WRITEFUNCTION);

        APISET().AddConst("CURLINFO_RESPONSE_CODE", CURLINFO_RESPONSE_CODE);
        APISET().AddConst("CURLINFO_TOTAL_TIME", CURLINFO_TOTAL_TIME);
        APISET().AddConst("CURLINFO_CONTENT_TYPE", CURLINFO_CONTENT_TYPE);
        APISET().AddConst("CURLINFO_EFFECTIVE_URL", CURLINFO_EFFECTIVE_URL);
        END_PACKAGE

        Curl();
        ~Curl();

        // Set a libcurl option. The key is typically a string representing the option name,
        // and the value can be a boolean, integer, or string.
        bool SetOpt(X::Value key, X::Value value);
        // Perform the HTTP request.
        bool Perform();
        // Retrieve information (e.g. RESPONSE_CODE, TOTAL_TIME, etc.) after performing a request.
        X::Value GetInfo(const std::string& infoName);
        // Reset the curl handle to its initial state.
        void Reset();

        // Getters for the response and error buffers.
        std::string GetResponse() const { return m_response; }
        std::string GetError() const { return m_error; }

        // Static helper functions for URL escaping/unescaping.
        std::string Escape(const std::string& input);
        std::string Unescape(const std::string& input);
    private:
        // Callback used by libcurl to write response data.
        static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp);
    };
}
