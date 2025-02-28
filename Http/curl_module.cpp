/*
Copyright (C) 2024 The XLang Foundation
Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0
*/

#include "curl_module.h"
#include <cstring>
#include <iostream>

namespace X {

    Curl::Curl()
        : m_curl(curl_easy_init()),
        m_response(""),
        m_error(""),
        m_headerList(nullptr)
    {
        memset(m_errorBuffer, 0, sizeof(m_errorBuffer));
        if (m_curl)
        {
            // Set the write callback to capture response data.
            curl_easy_setopt(m_curl, CURLOPT_WRITEFUNCTION, Curl::WriteCallback);
            curl_easy_setopt(m_curl, CURLOPT_WRITEDATA, m_response.c_str());
            // Set an error buffer.
            curl_easy_setopt(m_curl, CURLOPT_ERRORBUFFER, m_errorBuffer);
        }
    }

    Curl::~Curl()
    {
        if (m_headerList)
        {
            curl_slist_free_all(m_headerList);
            m_headerList = nullptr;
        }
        if (m_curl)
        {
            curl_easy_cleanup(m_curl);
            m_curl = nullptr;
        }
    }

    size_t Curl::WriteCallback(void* contents, size_t size, size_t nmemb, void* userp)
    {
        size_t totalSize = size * nmemb;
        Curl* self = static_cast<Curl*>(userp);
        self->m_response.append(static_cast<char*>(contents), totalSize);
        return totalSize;
    }

    bool Curl::SetOpt(X::Value key, X::Value value)
    {
        if (!m_curl)
            return false;

        // Determine the numeric option code.
        long optCode = 0;
        if (key.IsLong()) {
            // If key is a number, use it directly.
            optCode = key.ToInt();
        }
        else if (key.IsString()) {
            std::string option = key.ToString();
            // Allow both with and without the "CURLOPT_" prefix.
            if (option == "URL" || option == "CURLOPT_URL")
                optCode = CURLOPT_URL;
            else if (option == "FOLLOWLOCATION" || option == "CURLOPT_FOLLOWLOCATION")
                optCode = CURLOPT_FOLLOWLOCATION;
            else if (option == "TIMEOUT" || option == "CURLOPT_TIMEOUT")
                optCode = CURLOPT_TIMEOUT;
            else if (option == "USERAGENT" || option == "CURLOPT_USERAGENT")
                optCode = CURLOPT_USERAGENT;
            else if (option == "POSTFIELDS" || option == "CURLOPT_POSTFIELDS")
                optCode = CURLOPT_POSTFIELDS;
            else if (option == "HTTPHEADER" || option == "CURLOPT_HTTPHEADER")
                optCode = CURLOPT_HTTPHEADER;
            else if (option == "POST" || option == "CURLOPT_POST")
                optCode = CURLOPT_POST;
            else if (option == "CUSTOMREQUEST" || option == "CURLOPT_CUSTOMREQUEST")
                optCode = CURLOPT_CUSTOMREQUEST;
            else if (option == "VERBOSE" || option == "CURLOPT_VERBOSE")
                optCode = CURLOPT_VERBOSE;
            else if (option == "PROXY" || option == "CURLOPT_PROXY")
                optCode = CURLOPT_PROXY;
            else if (option == "PROXYPORT" || option == "CURLOPT_PROXYPORT")
                optCode = CURLOPT_PROXYPORT;
            else if (option == "COOKIE" || option == "CURLOPT_COOKIE")
                optCode = CURLOPT_COOKIE;
            else if (option == "SSL_VERIFYPEER" || option == "CURLOPT_SSL_VERIFYPEER")
                optCode = CURLOPT_SSL_VERIFYPEER;
            else if (option == "SSL_VERIFYHOST" || option == "CURLOPT_SSL_VERIFYHOST")
                optCode = CURLOPT_SSL_VERIFYHOST;
            else {
                m_error = "Unsupported option string: " + option;
                return false;
            }
        }
        else {
            m_error = "Unsupported key type, must be integer or string.";
            return false;
        }

        CURLcode res = CURLE_OK;
        switch (optCode) {
        case CURLOPT_URL:
            res = curl_easy_setopt(m_curl, CURLOPT_URL, value.ToString().c_str());
            break;
        case CURLOPT_FOLLOWLOCATION:
            res = curl_easy_setopt(m_curl, CURLOPT_FOLLOWLOCATION, value.ToBool() ? 1L : 0L);
            break;
        case CURLOPT_TIMEOUT:
            res = curl_easy_setopt(m_curl, CURLOPT_TIMEOUT, static_cast<long>(value.ToInt()));
            break;
        case CURLOPT_USERAGENT:
            res = curl_easy_setopt(m_curl, CURLOPT_USERAGENT, value.ToString().c_str());
            break;
        case CURLOPT_POSTFIELDS:
            // Note: The lifetime of the passed string must be managed by the caller.
            res = curl_easy_setopt(m_curl, CURLOPT_POSTFIELDS, value.ToString().c_str());
            break;
        case CURLOPT_HTTPHEADER:
            // Clear any previous header list.
            if (m_headerList) {
                curl_slist_free_all(m_headerList);
                m_headerList = nullptr;
            }
            // Accept a list of strings or a single string.
            if (value.IsList()) {
                X::List headerList(value);
                for (auto h : *headerList) {
                    std::string header = h.ToString();
                    m_headerList = curl_slist_append(m_headerList, header.c_str());
                }
            }
            else {
                m_headerList = curl_slist_append(nullptr, value.ToString().c_str());
            }
            res = curl_easy_setopt(m_curl, CURLOPT_HTTPHEADER, m_headerList);
            break;
        case CURLOPT_POST:
            res = curl_easy_setopt(m_curl, CURLOPT_POST, value.ToBool() ? 1L : 0L);
            break;
        case CURLOPT_CUSTOMREQUEST:
            res = curl_easy_setopt(m_curl, CURLOPT_CUSTOMREQUEST, value.ToString().c_str());
            break;
        case CURLOPT_VERBOSE:
            res = curl_easy_setopt(m_curl, CURLOPT_VERBOSE, value.ToBool() ? 1L : 0L);
            break;
        case CURLOPT_PROXY:
            res = curl_easy_setopt(m_curl, CURLOPT_PROXY, value.ToString().c_str());
            break;
        case CURLOPT_PROXYPORT:
            res = curl_easy_setopt(m_curl, CURLOPT_PROXYPORT, static_cast<long>(value.ToInt()));
            break;
        case CURLOPT_COOKIE:
            res = curl_easy_setopt(m_curl, CURLOPT_COOKIE, value.ToString().c_str());
            break;
        case CURLOPT_SSL_VERIFYPEER:
            // Disable or enable certificate verification.
            res = curl_easy_setopt(m_curl, CURLOPT_SSL_VERIFYPEER, value.ToBool() ? 1L : 0L);
            break;
        case CURLOPT_SSL_VERIFYHOST:
            // For CURLOPT_SSL_VERIFYHOST, 2 means verify the host and 0 means do not verify.
            res = curl_easy_setopt(m_curl, CURLOPT_SSL_VERIFYHOST, value.ToBool() ? 2L : 0L);
            break;
        default:
            m_error = "Unsupported option code: " + std::to_string(optCode);
            return false;
        }

        if (res != CURLE_OK) {
            m_error = (m_errorBuffer[0] != '\0') ? m_errorBuffer : curl_easy_strerror(res);
            return false;
        }
        return true;
    }

    bool Curl::Perform()
    {
        if (!m_curl)
            return false;

        // Clear previous response and error.
        m_response.clear();
        m_error.clear();
        memset(m_errorBuffer, 0, sizeof(m_errorBuffer));

        CURLcode res = curl_easy_perform(m_curl);
        if (res != CURLE_OK)
        {
            m_error = (m_errorBuffer[0] != '\0') ? m_errorBuffer : curl_easy_strerror(res);
            return false;
        }
        return true;
    }

    X::Value Curl::GetInfo(const std::string& infoName)
    {
        if (!m_curl)
            return X::Value(false);

        if (infoName == "RESPONSE_CODE")
        {
            long code = 0;
            curl_easy_getinfo(m_curl, CURLINFO_RESPONSE_CODE, &code);
            return X::Value((int)code);
        }
        else if (infoName == "TOTAL_TIME")
        {
            double t = 0.0;
            curl_easy_getinfo(m_curl, CURLINFO_TOTAL_TIME, &t);
            return X::Value(t);
        }
        else if (infoName == "CONTENT_TYPE")
        {
            char* ct = nullptr;
            curl_easy_getinfo(m_curl, CURLINFO_CONTENT_TYPE, &ct);
            return X::Value(ct ? std::string(ct) : std::string(""));
        }
        else if (infoName == "EFFECTIVE_URL")
        {
            char* url = nullptr;
            curl_easy_getinfo(m_curl, CURLINFO_EFFECTIVE_URL, &url);
            return X::Value(url ? std::string(url) : std::string(""));
        }
        return X::Value(false);
    }

    void Curl::Reset()
    {
        if (m_curl)
        {
            curl_easy_reset(m_curl);
            memset(m_errorBuffer, 0, sizeof(m_errorBuffer));
            curl_easy_setopt(m_curl, CURLOPT_ERRORBUFFER, m_errorBuffer);
            curl_easy_setopt(m_curl, CURLOPT_WRITEFUNCTION, Curl::WriteCallback);
            curl_easy_setopt(m_curl, CURLOPT_WRITEDATA, this);
        }
        if (m_headerList)
        {
            curl_slist_free_all(m_headerList);
            m_headerList = nullptr;
        }
        m_response.clear();
        m_error.clear();
    }

    std::string Curl::Escape(const std::string& input)
    {
        // Create a temporary handle for escaping.
        CURL* curlHandle = curl_easy_init();
        if (curlHandle)
        {
            char* output = curl_easy_escape(curlHandle, input.c_str(), (int)input.length());
            std::string result = output ? output : "";
            curl_free(output);
            curl_easy_cleanup(curlHandle);
            return result;
        }
        return "";
    }

    std::string Curl::Unescape(const std::string& input)
    {
        // Create a temporary handle for unescaping.
        CURL* curlHandle = curl_easy_init();
        if (curlHandle)
        {
            int outLength = 0;
            char* output = curl_easy_unescape(curlHandle, input.c_str(), (int)input.length(), &outLength);
            std::string result = output ? output : "";
            curl_free(output);
            curl_easy_cleanup(curlHandle);
            return result;
        }
        return "";
    }

} // namespace X
