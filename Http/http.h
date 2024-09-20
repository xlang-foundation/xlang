#pragma once
#include "xpackage.h"
#include "xlang.h"
#include <vector>
#include <string>
#include <iostream>
#include <regex>
#include "singleton.h"

namespace X
{
	struct UrlPattern
	{
		std::string strRule;
		std::regex rule;
		X::ARGS params;
		X::KWARGS kwParams;
		X::Value handler;
	};
	class HttpServer
	{
		std::vector<UrlPattern> m_patters;

		void* m_pSrv = nullptr;
		bool m_bAsHttps = false;
		std::string m_cert_path;
		std::string m_private_key_path;
		std::string m_client_ca_cert_file_path;
		std::string m_client_ca_cert_dir_path;
		std::string m_private_key_password;
		std::vector<void*> m_handlers;
		bool m_SupportStaticFiles = true;
		std::string m_staticIndexFile = "index.html";
		std::vector<std::string> m_staticFileRoots;

		std::string TranslateUrlToReqex(std::string& url);

		bool HandleStaticFile(std::string path, void* pReq, void* pResp);
		std::string GetModulePath();
	
		std::string ConvertReletivePathToFullPath(std::string path);
	public:
		BEGIN_PACKAGE(HttpServer)
			APISET().AddEvent("OnConnect");
			APISET().AddPropWithType<bool>("SupportStaticFiles", &HttpServer::m_SupportStaticFiles);
			APISET().AddPropWithType<std::string>("StaticIndexFile", &HttpServer::m_staticIndexFile);
			APISET().AddPropL("StaticRoots",
				[](auto* pThis, X::Value v) {
					if (v.IsList())
					{
						X::List l(v);
						for (auto item : *l)
						{
							std::string strPath = item.ToString();
							std::string steAbsolutePath = pThis->ConvertReletivePathToFullPath(strPath);
							pThis->m_staticFileRoots.push_back(steAbsolutePath);
						}
					}
					else
					{
						std::string strPath = v.ToString();
						std::string steAbsolutePath = pThis->ConvertReletivePathToFullPath(strPath);
						pThis->m_staticFileRoots.push_back(steAbsolutePath);
					}
				},
				[](auto* pThis) {
					X::List l;
					for (auto& root : pThis->m_staticFileRoots)
					{
						l+=root;
					}
					return l;
				});
			APISET().AddFunc<2>("listen", &HttpServer::Listen);
			APISET().AddFunc<0>("stop", &HttpServer::Stop);
			APISET().AddFunc<2>("get", &HttpServer::Get);
			APISET().AddVarFuncEx("route", &HttpServer::Route);
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
		bool Route(X::XRuntime* rt, X::XObj* pThis,X::XObj* pContext,
			X::ARGS& params, X::KWARGS& kwParams,
			X::Value& trailer, X::Value& retValue);
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
			APISET().AddFunc<2>("add_header", &HttpResponse::AddHeader);
		END_PACKAGE
		HttpResponse()
		{
		}
		HttpResponse(void* pResp):HttpResponse()
		{
			m_pResponse = pResp;
		}
		bool SetContent(X::Value& valContent, std::string contentType);
		bool AddHeader(std::string headName,X::Value& headValue);
	};
	class HttpClient
	{
		void* m_pClient = nullptr;
		bool m_isHttps = false;
		int m_status = 0;
		X::Value m_body;
		X::Dict m_headers;
		X::Value m_response_headers;
		void set_enable_server_certificate_verification(bool b);
	public:
		BEGIN_PACKAGE(HttpClient)
			APISET().AddFunc<1>("get", &HttpClient::Get);
			APISET().AddFunc<3>("post", &HttpClient::Post);
			APISET().AddProp0("headers", &HttpClient::m_headers);
			APISET().AddPropL("enable_server_certificate_verification", 
				[](auto* pThis, X::Value v) {
					pThis->set_enable_server_certificate_verification((bool)v);
				},[](auto* pThis) {return false; });

			APISET().AddPropL("status",[](auto* pThis, X::Value v) {},
				[](auto* pThis){return pThis->GetStatus(); });
			APISET().AddPropL("response_headers", [](auto* pThis, X::Value v) {},
				[](auto* pThis) {return pThis->GetResponseHeaders(); });
			APISET().AddPropL("body", [](auto* pThis, X::Value v) {},
				[](auto* pThis) {return pThis->GetBody(); });
		END_PACKAGE
		HttpClient(std::string url);
		~HttpClient();
		bool Get(std::string path);
		bool Post(std::string path, std::string content_type, std::string body);
		X::Value GetStatus();
		X::Value GetBody();
		X::Value GetResponseHeaders() { return m_response_headers; }
	};
	class Http:
		public Singleton<Http>
	{
		//for calling XModule
		X::Value m_curModule;
		std::string m_curModulePath;
		//for this http module
		std::string m_httpModulePath;
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
		std::string& GetModulePath()
		{
			return m_curModulePath;
		}
		void SetHttpModulePath(std::string path)
		{
			m_httpModulePath = path;
		}
		std::string& GetHttpModulePath()
		{
			return m_httpModulePath;
		}
		void SetModule(X::Value curModule)
		{
			X::XModule* pModule = dynamic_cast<X::XModule*>(curModule.GetObj());
			if (pModule)
			{
				auto path = pModule->GetPath();
				m_curModulePath = path;
				g_pXHost->ReleaseString(path);
			}
			m_curModule = curModule;
		}

	};
}