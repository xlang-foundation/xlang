#pragma once
#include "xlang.h"
namespace X
{
	class HttpServer
	{
		void* m_pSrv = nullptr;
	public:
		BEGIN_PACKAGE(HttpServer)
			ADD_FUNC("listen", Listen)
			ADD_FUNC("stop", Stop)
			ADD_FUNC("get", Get)
		END_PACKAGE

		HttpServer(ARGS& params,
			KWARGS& kwParams);
		bool Listen(void* rt,void* pContext,
			ARGS& params,
			KWARGS& kwParams,
			AST::Value& retValue);
		bool Stop(void* rt, void* pContext,
			ARGS& params,
			KWARGS& kwParams,
			AST::Value& retValue);
		bool Get(void* rt, void* pContext,ARGS& params,
			KWARGS& kwParams,
			AST::Value& retValue);
	};
	class HttpResponse
	{
		void* m_pResponse = nullptr;
	public:
		BEGIN_PACKAGE(HttpResponse)
			ADD_FUNC("set_content",SetContent)
		END_PACKAGE
		HttpResponse(void* pResp)
		{
			m_pResponse = pResp;
		}
		HttpResponse(ARGS& params, KWARGS& kwParams)
		{

		}
		bool SetContent(void* rt, void* pContext,
			ARGS& params,
			KWARGS& kwParams,
			AST::Value& retValue);
	};

	class Http
	{
	public:
		BEGIN_PACKAGE(Http)
			ADD_CLASS("Server", HttpServer)
			ADD_CLASS("Response", HttpResponse)
		END_PACKAGE
		Http()
		{

		}
	};
}