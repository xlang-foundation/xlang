#include "http.h"
#include "httplib.h"


namespace X
{
	void HttpServer::Init()
	{
		httplib::Server* pSrv = new httplib::Server();
		if (!pSrv->is_valid())
		{
			printf("server has an error...\n");
		}
		m_pSrv = (void*)pSrv;
	}
	HttpServer::~HttpServer()
	{
		for (auto p : m_handlers)
		{
			XFunc* pFuncObj = (XFunc*)p;
			pFuncObj->DecRef();
		}
		m_handlers.clear();
	}
	bool HttpServer::Listen(std::string srvName,int port)
	{
		//Fire(0, params, kwParams);//event test
		bool bOK = ((httplib::Server*)m_pSrv)->listen(
			srvName.c_str(), port);
		return bOK;
	}
	bool HttpServer::Stop()
	{
		((httplib::Server*)m_pSrv)->stop();
		return true;
	}
	bool HttpServer::Get(std::string pattern,X::Value& valHandler)
	{
		XFunc* pHandler = nullptr;
		if (valHandler.IsObject())
		{
			auto* pFuncObj = valHandler.GetObj();
			if (pFuncObj->GetType() == X::ObjType::Function)
			{
				pHandler = dynamic_cast<XFunc*>(pFuncObj);
				pHandler->IncRef();//keep for lambda
				m_handlers.push_back((void*)pHandler);
			}
		}
		XPackage* pCurPack = APISET().GetPack();
		((httplib::Server*)m_pSrv)->Get(pattern,
			[pHandler, pCurPack](const httplib::Request& req,
				httplib::Response& res)
			{
				if (pHandler)
				{
					ARGS params0;
					HttpRequest* pHttpReq = new HttpRequest((void*)&req);
					params0.push_back(X::Value(pHttpReq->APISET().GetPack()));

					HttpResponse* pHttpResp = new HttpResponse(&res);
					params0.push_back(X::Value(pHttpReq->APISET().GetPack()));

					KWARGS kwParams0;
					X::Value retValue0;
					try 
					{
						pHandler->Call(X::g_pXHost->GetCurrentRuntime(),
							pCurPack,
							params0, kwParams0,
							retValue0);
					}
					catch (int e)
					{
						std::cout << "An exception occurred. Exception Nr. " << e << '\n';
					}
					catch (...)
					{
						std::cout << "An exception occurred."<< '\n';
					}
				}
			});
		return true;
	}
	bool HttpResponse::SetContent(X::Value& valContent,std::string contentType)
	{
		auto* pResp = (httplib::Response*)m_pResponse;
		if (valContent.IsObject())
		{
			auto* pObjContent = valContent.GetObj();
			if (pObjContent->GetType() == X::ObjType::Binary)
			{
				auto* pBinContent = dynamic_cast<XBin*>(pObjContent);
				pBinContent->IncRef();
				pResp->set_content_provider(
					pBinContent->Size(), // Content length
					contentType.c_str(), // Content type
					[pBinContent](size_t offset, size_t length, 
						httplib::DataSink& sink) 
					{
						char* data = pBinContent->Data();
						sink.write(data+offset,length);
						return true;
					},
					[pBinContent](bool success) 
					{ 
						pBinContent->DecRef();
					}
					);
			}
			else
			{
				pResp->set_content(valContent.ToString().c_str(),
					contentType.c_str());
			}
		}
		else
		{
			pResp->set_content(valContent.ToString().c_str(),
				contentType.c_str());
		}
		return true;
	}
	X::Value HttpRequest::GetMethod()
	{
		auto* pReq = (httplib::Request*)m_pRequest; 
		std::string strVal = pReq->method;
		return X::Value((char*)strVal.c_str(), (int)strVal.size()); 
	}
	X::Value  HttpRequest::GetBody()
	{
		auto* pReq = (httplib::Request*)m_pRequest;
		std::string strVal = pReq->body;
		return X::Value((char*)strVal.c_str(), (int)strVal.size());
	}
	X::Value  HttpRequest::GetPath()
	{
		auto* pReq = (httplib::Request*)m_pRequest;
		std::string strVal = pReq->path;
		return X::Value((char*)strVal.c_str(), (int)strVal.size());
	}
	X::Value  HttpRequest::Get_remote_addr()
	{
		auto* pReq = (httplib::Request*)m_pRequest;
		std::string strVal = pReq->remote_addr;
		return X::Value((char*)strVal.c_str(), (int)strVal.size());
	}
	X::Value  HttpRequest::GetAllHeaders()
	{
		auto* pReq = (httplib::Request*)m_pRequest;
		auto& headers = pReq->headers;
		XDict dict;
		for (auto it = headers.begin(); it != headers.end(); ++it)
		{
			const auto& x = *it;
			X::Value key(XStr(x.first.c_str(), (int)x.first.size()));
			X::Value val(XStr(x.second.c_str(), (int)x.second.size()));
			dict.Set(key, val);
		}
		return X::Value(dict);
	}
	X::Value  HttpRequest::GetParams()
	{
		auto* pReq = (httplib::Request*)m_pRequest;
		auto& req_params = pReq->params;
		XDict dict;
		for (auto it = req_params.begin();
			it != req_params.end(); ++it)
		{
			const auto& x = *it;
			X::Value key(XStr(x.first.c_str(), (int)x.first.size()));
			X::Value val(XStr(x.second.c_str(), (int)x.second.size()));
			dict.Set(key, val);
		}
		return X::Value(dict);
	}
}

