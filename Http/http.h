#pragma once
#include "xpackage.h"
#include "xlang.h"
#include <vector>

namespace X
{
	class HttpServer
	{
		void* m_pSrv = nullptr;
		std::vector<void*> m_handlers;
	public:
		BEGIN_PACKAGE(HttpServer)
			ADD_FUNC("listen", Listen)
			ADD_FUNC("stop", Stop)
			ADD_FUNC("get", Get)
		END_PACKAGE

		HttpServer(ARGS& params,
			KWARGS& kwParams);
		~HttpServer();
		bool Listen(void* rt,XObj* pContext,
			ARGS& params,
			KWARGS& kwParams,
			X::Value& retValue);
		bool Stop(void* rt, XObj* pContext,
			ARGS& params,
			KWARGS& kwParams,
			X::Value& retValue);
		bool Get(void* rt, XObj* pContext,ARGS& params,
			KWARGS& kwParams,
			X::Value& retValue);
	};
#define GET_FUNC(name) \
	bool Get##name(void* rt, XObj* pContext,\
		ARGS& params,\
		KWARGS& kwParams,\
		X::Value& retValue);

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

		bool GetAllHeaders(void* rt, XObj* pContext,
			ARGS& params,
			KWARGS& kwParams,
			X::Value& retValue);
		bool GetParams(void* rt, XObj* pContext, ARGS& params,
			KWARGS& kwParams, X::Value& retValue);
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
		bool SetContent(void* rt, XObj* pContext,
			ARGS& params,
			KWARGS& kwParams,
			X::Value& retValue);
	};

	class Http
	{
	public:
		BEGIN_PACKAGE(Http)
			ADD_PROP(name)
			ADD_PROP(test)
			ADD_CLASS("Server", HttpServer)
			ADD_CLASS("Response", HttpResponse)
			ADD_CLASS("Request", HttpRequest)
		END_PACKAGE
	public:
		int test = 1234;
		std::string name;
		Http()
		{

		}
	};
}