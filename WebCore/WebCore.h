#pragma once
#include "xpackage.h"
#include "xlang.h"
#include "HttpClient.h"
#include "websocket_srv.h"

namespace X
{
    namespace WebCore
    {
		class WebCore
		{
		public:
			BEGIN_PACKAGE(WebCore)
				APISET().AddClass<1, HttpRequest>("HttpRequest");
				APISET().AddClass<1, WebSocketServer>("WebSocketServer");
			END_PACKAGE
		};
	}
}