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
			APISET().AddEvent("OnConnect");
			APISET().AddPropWithType<std::string>("name", &HttpServer::name);
			APISET().AddProp0("test", &HttpServer::test);
			APISET().AddFunc<2>("listen", &HttpServer::Listen);
			APISET().AddFunc<0>("stop", &HttpServer::Stop);
			APISET().AddFunc<2>("get", &HttpServer::Get);
		END_PACKAGE
	public:

		int test = 1234;
		std::string name;

		HttpServer()
		{
			Init();
		}
		~HttpServer();
		void Init();
		bool Listen(std::string srvName, int port);
		bool Stop();
		bool Get(std::string pattern, X::Value& valHandler);
	};
	class HttpRequest
	{
		void* m_pRequest = nullptr;

	public:
		BEGIN_PACKAGE(HttpRequest)
			APISET().AddProp("params", &HttpRequest::GetParams);
			APISET().AddProp("all_headers", &HttpRequest::GetAllHeaders);
			APISET().AddProp("body", &HttpRequest::GetBody);
			APISET().AddProp("method", &HttpRequest::GetMethod);
			APISET().AddProp("path", &HttpRequest::GetPath);
			APISET().AddProp("remote_addr", &HttpRequest::Get_remote_addr);
		END_PACKAGE
		HttpRequest()
		{
		}
		HttpRequest(void* pReq):HttpRequest()
		{
			m_pRequest = pReq;
		}
		X::Value GetMethod();
		X::Value GetBody();
		X::Value GetPath();
		X::Value Get_remote_addr();
		X::Value GetAllHeaders();
		X::Value GetParams();
	};
	class HttpResponse
	{
		void* m_pResponse = nullptr;
	public:
		BEGIN_PACKAGE(HttpResponse)
			APISET().AddFunc<2>("set_content", &HttpResponse::SetContent);
		END_PACKAGE
		HttpResponse()
		{
		}
		HttpResponse(void* pResp):HttpResponse()
		{
			m_pResponse = pResp;
		}
		bool SetContent(X::Value& valContent, std::string contentType);
	};
	class HttpClient
	{
		void* m_pClient = nullptr;
		int m_status = 0;
		X::Value m_body;
	public:
		BEGIN_PACKAGE(HttpClient)
			APISET().AddFunc<1>("get", &HttpClient::Get);
			APISET().AddPropL("status",[](auto* pThis, X::Value v) {},
				[](auto* pThis){return pThis->GetStatus(); });
			APISET().AddPropL("body", [](auto* pThis, X::Value v) {},
				[](auto* pThis) {return pThis->GetBody(); });
		END_PACKAGE
		HttpClient(std::string scheme_host_port);
		~HttpClient();
		bool Get(std::string path);
		X::Value GetStatus();
		X::Value GetBody();
	};
	class Http
	{
	public:
		BEGIN_PACKAGE(Http)
			APISET().AddClass<0, HttpServer>("Server");
			APISET().AddClass<0, HttpResponse>("Response");
			APISET().AddClass<0, HttpRequest>("Request");
			APISET().AddClass<1, HttpClient>("Client");
		END_PACKAGE
	};
}