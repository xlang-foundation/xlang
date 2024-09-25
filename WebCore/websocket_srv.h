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
		class WebSocketSession
		{
			void* m_pImpl = nullptr;
		public:
			BEGIN_PACKAGE(WebSocketSession)
				APISET().AddFunc<1>("write", &WebSocketSession::Write);
			END_PACKAGE
			WebSocketSession();
			~WebSocketSession();
			FORCE_INLINE void SetImpl(void* pImpl)
			{
				m_pImpl = pImpl;
			}
			bool Write(X::Value& value);
		};
		class WebSocketServer
		{
			void* m_pImpl = nullptr;
		public:
			BEGIN_PACKAGE(WebSocketServer)
				APISET().AddClass<0, WebSocketSession>("WebSocketSession");
				APISET().AddEvent("OnAddSession");
				APISET().AddEvent("OnSessionReceive");
				APISET().AddEvent("OnRemoveSession");
			END_PACKAGE
			WebSocketServer(int port);
			~WebSocketServer();
		};
	}
}