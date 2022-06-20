#include "http.h"
#include "httplib.h"
#include "object.h"

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
			}
		}
		((httplib::Server*)m_pSrv)->Get(pattern,
			[=](const httplib::Request& req, 
				httplib::Response& res)
			{
				if (pHandler)
				{
					ARGS params0;
					HttpResponse* pHttpResp 
						= new HttpResponse(&res);
					X::AST::Package* pPackage = nullptr;
					pHttpResp->Create((X::Runtime*)rt,
						&pPackage);
					params0.push_back(AST::Value(pPackage));
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
		pResp->set_content(params[0].ToString().c_str(),
			params[1].ToString().c_str());
		return true;
	}
}
