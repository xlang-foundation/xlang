/*
Copyright (C) 2024 The XLang Foundation
Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

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
