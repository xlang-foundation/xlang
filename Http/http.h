#pragma once
#include "xpackage.h"
#include "xlang.h"
#include <vector>
#include <string>
#include <iostream>

namespace X
{
	class HttpServer
	{
		void* m_pSrv = nullptr;
		bool m_bAsHttps = false;
		std::string m_cert_path;
		std::string m_private_key_path;
		std::string m_client_ca_cert_file_path;
		std::string m_client_ca_cert_dir_path;
		std::string m_private_key_password;
		std::vector<void*> m_handlers;
	public:
		BEGIN_PACKAGE(HttpServer)
			APISET().AddEvent("OnConnect");
			APISET().AddFunc<2>("listen", &HttpServer::Listen);
			APISET().AddFunc<0>("stop", &HttpServer::Stop);
			APISET().AddFunc<2>("get", &HttpServer::Get);
		END_PACKAGE
	public:

		HttpServer(X::ARGS& params, X::KWARGS& kwParams)
		{
			if (params.size() > 0)
			{
				m_bAsHttps = true;
				m_cert_path = params[0].ToString();
				if (params.size() > 1)
				{
					m_private_key_path = params[1].ToString();
				}
				if (params.size() > 2)
				{
					m_client_ca_cert_file_path = params[2].ToString();
				}
				if (params.size() > 3)
				{
					m_client_ca_cert_dir_path = params[3].ToString();
				}
				if (params.size() > 4)
				{
					m_private_key_password = params[4].ToString();
				}
			}
			Init(m_bAsHttps);
		}
		~HttpServer();
		void Init(bool asHttps);
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
			APISET().AddFunc<1>("WritePad", &Http::WritePad);
			APISET().AddVarClass<HttpServer>("Server",
				"for http server,no parameters,for https with ssl,"
				"[cert_path,private_key_path,client_ca_cert_file_path,"
				"client_ca_cert_dir_path,private_key_password]");
			APISET().AddClass<0, HttpResponse>("Response");
			APISET().AddClass<0, HttpRequest>("Request");
			APISET().AddClass<1, HttpClient>("Client");
		END_PACKAGE
		X::Value WritePad(X::Value& input);
	};
}