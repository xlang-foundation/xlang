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
#define GET_FUNC(name) \
	bool Get##name(void* rt, void* pContext,\
		ARGS& params,\
		KWARGS& kwParams,\
		AST::Value& retValue);

	class HttpRequest
	{
		void* m_pRequest = nullptr;
	public:
		BEGIN_PACKAGE(HttpRequest)
			ADD_FUNC("get_params", GetParams)
			ADD_FUNC("get_all_headers", GetAllHeaders)
			ADD_FUNC("get_body", Getbody)
			ADD_FUNC("get_method", Getmethod)
			ADD_FUNC("get_path", Getpath)
			ADD_FUNC("get_remote_addr", Getremote_addr)
		END_PACKAGE
		HttpRequest(void* pReq)
		{
			m_pRequest = pReq;
		}
		HttpRequest(ARGS& params, KWARGS& kwParams)
		{

		}
		GET_FUNC(method)
		GET_FUNC(body)
		GET_FUNC(path)
		GET_FUNC(remote_addr)

		bool GetAllHeaders(void* rt, void* pContext,
			ARGS& params,
			KWARGS& kwParams,
			AST::Value& retValue);
		bool GetParams(void* rt, void* pContext, ARGS& params,
			KWARGS& kwParams, AST::Value& retValue);
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
			ADD_CLASS("Request", HttpRequest)
			END_PACKAGE
		Http()
		{

		}
	};
}