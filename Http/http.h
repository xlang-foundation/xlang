#pragma once
#include "xpackage.h"
#include "xlang.h"
#include <vector>

namespace X
{
	class HttpServer
	{
		XPackageAPISet<HttpServer> m_Apis;
		void* m_pSrv = nullptr;
		std::vector<void*> m_handlers;
	public:
		XPackageAPISet<HttpServer>& APISET() { return m_Apis; }

		int test = 1234;
		std::string name;

		HttpServer()
		{
			m_Apis.AddEvent("OnConnect");
			m_Apis.AddProp0("name", &HttpServer::name);
			m_Apis.AddProp0("test", &HttpServer::test);
			m_Apis.AddFunc<2>("listen", &HttpServer::Listen);
			m_Apis.AddFunc<0>("stop", &HttpServer::Stop);
			m_Apis.AddFunc<2>("get", &HttpServer::Get);
			m_Apis.Create(this);
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
		XPackageAPISet<HttpRequest> m_Apis;
		void* m_pRequest = nullptr;
	public:
		XPackageAPISet<HttpRequest>& APISET() { return m_Apis; }
		HttpRequest()
		{
			m_Apis.AddProp("params", &HttpRequest::GetParams);
			m_Apis.AddProp("all_headers",&HttpRequest::GetAllHeaders);
			m_Apis.AddProp("body", &HttpRequest::GetBody);
			m_Apis.AddProp("method",&HttpRequest::GetMethod);
			m_Apis.AddProp("path", &HttpRequest::GetPath);
			m_Apis.AddProp("remote_addr",&HttpRequest::Get_remote_addr);
			m_Apis.Create(this);
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
		XPackageAPISet<HttpResponse> m_Apis;
		void* m_pResponse = nullptr;
	public:
		XPackageAPISet<HttpResponse>& APISET() { return m_Apis; }
		HttpResponse()
		{
			m_Apis.AddFunc<2>("set_content", &HttpResponse::SetContent);
			m_Apis.Create(this);
		}
		HttpResponse(void* pResp):HttpResponse()
		{
			m_pResponse = pResp;
		}
		bool SetContent(X::Value& valContent, std::string contentType);
	};

	class Http
	{
		XPackageAPISet<Http> m_Apis;
	public:
		XPackageAPISet<Http>& APISET() { return m_Apis; }
		Http()
		{
			m_Apis.AddClass<0,HttpServer>("Server");
			m_Apis.AddClass<0, HttpResponse>("Response");
			m_Apis.AddClass<0, HttpRequest>("Request");
			m_Apis.Create(this);
		}
	};
}