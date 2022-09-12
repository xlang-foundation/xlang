#include "devsrv.h"
#include "xlang.h"
#include "Locker.h"
#include "wait.h"
#include <iostream>

namespace X
{
	DevServer::DevServer(int port):
		m_port(port)
	{
	}
	DevServer::~DevServer()
	{
	}
	void DevServer::run()
	{
		std::vector<std::string> notis;
		Locker notiLock;
		XWait notiWait;
		long dbg_handler_cookie = X::OnEvent("devops.dbg", 
			[this, &notis, &notiLock,&notiWait](
				XRuntime* rt, XObj* pContext,
				ARGS& params, KWARGS& kwParams,
				X::Value& retValue)
		{
			auto action = kwParams["action"];
			std::string notifyInfo;
			if (action == "end")
			{
				notifyInfo = "end";
			}
			else if (action == "notify")
			{
				auto param = kwParams["param"];
				notifyInfo = "$notify$" + param.ToString();
			}
			notiLock.Lock();
			notis.push_back(notifyInfo);
			notiLock.Unlock();
			notiWait.Release();
			//std::cout << "DevServer got event:" << notifyInfo<<std::endl;
		});
		m_srv.Get("/devops/run",
			[this](const httplib::Request& req,httplib::Response& res)
			{
				auto& req_params = req.params;
				auto it = req_params.find("code");
				std::string retData;
				if (it != req_params.end())
				{
					std::string code = it->second;
					//std::cout << "code:\n" << code << std::endl;
					if (X::g_pXHost)
					{
						X::Value retVal;
						std::string moduleName("devops_run.x");
						X::g_pXHost->RunCode(moduleName, code, retVal);
						retData = retVal.ToString(true);
					}
				}
				else
				{
					retData = "error";
				}
				res.set_content(retData, "text/html");
				//std::cout << "BackData:" << retData << std::endl;
			}
		);
		m_srv.Get("/devops/getnotify",
			[this, &notis, &notiLock, &notiWait](
				const httplib::Request& req, httplib::Response& res)
			{
				std::string notifyInfo;
				notiLock.Lock();
				if (notis.size() == 0)
				{
					notiLock.Unlock();
					notiWait.Wait(-1);
					notiLock.Lock();
				}
				if (notis.size() > 0)
				{
					notifyInfo = notis[0];
					notis.erase(notis.begin());
				}
				notiLock.Unlock();
				res.set_content(notifyInfo, "text/html");
			}
		);
		if (!m_srv.is_valid())
		{
			printf("Devops server has an error...\n");
		}
		std::cout << "DevServer listens on port:" << m_port << std::endl;
		m_srv.listen("::", m_port);
		//exit
		notiWait.Release();
		X::OffEvent("devops.dbg", dbg_handler_cookie);
	}
}
