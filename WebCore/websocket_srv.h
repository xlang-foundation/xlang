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