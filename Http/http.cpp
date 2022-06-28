#include "http.h"
#include "httplib.h"
#include "object.h"
#include "dict.h"
#include "bin.h"
#include "str.h"

#define GET_FUNC_IMPL(name) \
bool HttpRequest::Get##name(void* rt, void* pContext,\
	ARGS& params,\
	KWARGS& kwParams,\
	AST::Value& retValue)\
{\
	auto* pReq = (httplib::Request*)m_pRequest;\
	std::string& strVal =  pReq->##name;\
	retValue = AST::Value((char*)strVal.c_str(), (int)strVal.size());\
	return true;\
}

namespace X
{
	HttpServer::HttpServer(ARGS& params,
		KWARGS& kwParams)
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
			Data::Function* pFuncObj = (Data::Function*)p;
			pFuncObj->Release();
		}
		m_handlers.clear();
	}
	bool HttpServer::Listen(void* rt, void* pContext,
		ARGS& params,
		KWARGS& kwParams,
		AST::Value& retValue)
	{
		std::string srvName = params[0].ToString();
		int port = (int)params[1].GetLongLong();
		bool bOK = ((httplib::Server*)m_pSrv)->listen(
			srvName.c_str(), port);
		retValue = AST::Value(bOK);
		return bOK;
	}
	bool HttpServer::Stop(void* rt, void* pContext,
		ARGS& params,
		KWARGS& kwParams,
		AST::Value& retValue)
	{
		((httplib::Server*)m_pSrv)->stop();
		return true;
	}
	bool HttpServer::Get(void* rt, void* pContext,
		ARGS& params,
		KWARGS& kwParams,
		AST::Value& retValue)
	{
		std::string pattern = params[0].ToString();
		Data::Function* pHandler = nullptr;
		if (params[1].IsObject())
		{
			Data::Object* pFuncObj
				= (Data::Object*)params[1].GetObj();
			if (pFuncObj->GetType() == X::Data::Type::Function)
			{
				pHandler = dynamic_cast<Data::Function*>(pFuncObj);
				pHandler->AddRef();//keep for lambda
				m_handlers.push_back((void*)pHandler);
			}
		}
		((httplib::Server*)m_pSrv)->Get(pattern,
			[pHandler,rt](const httplib::Request& req,
				httplib::Response& res)
			{
				if (pHandler)
				{
					ARGS params0;
					HttpRequest* pHttpReq
						= new HttpRequest((void*)&req);
					X::AST::Package* pPackageReq = nullptr;
					pHttpReq->Create((X::Runtime*)rt,
						&pPackageReq);
					params0.push_back(AST::Value(pPackageReq));

					HttpResponse* pHttpResp 
						= new HttpResponse(&res);
					X::AST::Package* pPackageResp = nullptr;
					pHttpResp->Create((X::Runtime*)rt,
						&pPackageResp);
					params0.push_back(AST::Value(pPackageResp));

					KWARGS kwParams0;
					AST::Value retValue0;
					pHandler->Call((X::Runtime*)rt,
						params0, kwParams0,
						retValue0);
				}
			});
		return true;
	}
	bool HttpResponse::SetContent(void* rt, void* pContext,
		ARGS& params, KWARGS& kwParams, AST::Value& retValue)
	{
		auto* pResp = (httplib::Response*)m_pResponse;
		AST::Value& valContent = params[0];
		if (valContent.IsObject())
		{
			Data::Object* pObjContent = (Data::Object*)valContent.GetObj();
			if (pObjContent->GetType() == Data::Type::Binary)
			{
				auto* pBinContent = dynamic_cast<Data::Binary*>(pObjContent);
				pBinContent->AddRef();
				pResp->set_content_provider(
					pBinContent->Size(), // Content length
					params[1].ToString().c_str(), // Content type
					[pBinContent](size_t offset, size_t length, 
						httplib::DataSink& sink) 
					{
						char* data = pBinContent->Data();
						sink.write(data+offset,length);
						return true;
					},
					[pBinContent](bool success) 
					{ 
						pBinContent->Release();
					}
					);
			}
			else
			{
				pResp->set_content(valContent.ToString().c_str(),
					params[1].ToString().c_str());
			}
		}
		else
		{
			pResp->set_content(valContent.ToString().c_str(),
				params[1].ToString().c_str());
		}
		return true;
	}
	GET_FUNC_IMPL(method)
	GET_FUNC_IMPL(body)
	GET_FUNC_IMPL(path)
	GET_FUNC_IMPL(remote_addr)
	bool HttpRequest::GetAllHeaders(void* rt, void* pContext, ARGS& params,
		KWARGS& kwParams, AST::Value& retValue)
	{
		auto* pReq = (httplib::Request*)m_pRequest;
		auto& headers = pReq->headers;
		Data::Dict* pDictObj = new Data::Dict();
		for (auto it = headers.begin(); it != headers.end(); ++it)
		{
			const auto& x = *it;
			AST::Value key(new Data::Str(x.first));
			AST::Value val(new Data::Str(x.second));
			pDictObj->Set(key, val);
		}
		retValue = AST::Value(pDictObj);
		return true;
	}
	bool HttpRequest::GetParams(void* rt, void* pContext, ARGS& params,
		KWARGS& kwParams, AST::Value& retValue)
	{
		auto* pReq = (httplib::Request*)m_pRequest;
		auto& req_params = pReq->params;
		Data::Dict* pDictObj = new Data::Dict();
		for (auto it = req_params.begin();
			it != req_params.end(); ++it)
		{
			const auto& x = *it;
			AST::Value key(new Data::Str(x.first));
			AST::Value val(new Data::Str(x.second));
			pDictObj->Set(key, val);
		}
		retValue = AST::Value(pDictObj);
		return true;
	}
}

