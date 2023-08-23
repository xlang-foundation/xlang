#pragma once
#include "xhost.h"
#include "xpackage.h"
#include "xlang.h"


namespace X 
{
    namespace WebCore
    {
        class HttpRequest
        {
            void* m_pImpl = nullptr;
        public:
            BEGIN_PACKAGE(HttpRequest)
                APISET().AddFunc<2>("setHeader", &HttpRequest::setHeader);
                APISET().AddFunc<1>("sendRequest", &HttpRequest::sendRequest);
                APISET().AddFunc<0>("readResponse", &HttpRequest::readResponse);
            END_PACKAGE
                HttpRequest(const std::string url);
            ~HttpRequest();
            bool setHeader(const std::string name, const std::string value);
            bool sendRequest(const std::string body_content);
            X::Value readResponse();
        };
    }
}
