#include "devsrv.h"
#include "xlang.h"
#include "Locker.h"
#include "wait.h"
#include <iostream>
#include <atomic>
#include <thread>
#include <chrono>

namespace X
{
	
	DevServer::DevServer(int port):
		m_port(port)
	{
		// if started by vscode and not connected in 10 seconds, exit xlang
		if (port >= 35000 && port < 36000)
		{
			std::thread connectionCheck([this] {
				std::unique_lock<std::mutex> lock(m_mtxConnect);
				m_cvConnect.wait_for(lock, std::chrono::seconds(10));
				if (!m_Connected) {
					m_srv.stop();
					std::cout << "no connection in 10 seconds, xlang exited" << std::endl;
					system("pause");
					std::exit(0);
				}
			});
			connectionCheck.detach();
		}
	}
	DevServer::~DevServer()
	{
	}
	void DevServer::run()
	{
		std::vector<std::string> notis;
		Locker notiLock;
		XWait notiWait;

		std::vector<std::string> printDataList;
		Locker printLock;
		XWait printWait;
		std::atomic_bool bCodelineRunning = true;

		long dbg_handler_cookie = X::OnEvent("devops.dbg", 
			[this, &notis, &notiLock,&notiWait](
				XRuntime* rt, XObj* pContext,
				ARGS& params, KWARGS& kwParams,
				X::Value& retValue)
		{
			std::string strAction;
			auto it0 = kwParams.find("action");
			if (it0)
			{
				strAction = it0->val.ToString();
			}
			std::string notifyInfo;
			if (strAction == "end")
			{
				notifyInfo = "end";
			}
			else if (strAction == "notify")
			{
				auto it2 = kwParams.find("param");
				if (it2)
				{
					notifyInfo = "$notify$" + it2->val.ToString();
				}
			}
			notiLock.Lock();
			notis.push_back(notifyInfo);
			notiLock.Unlock();
			notiWait.Release();
			//std::cout << "DevServer got event:" << notifyInfo<<std::endl;
		});
		long print_cookie = X::OnEvent("devops.print2jupyter",
			[this, &printDataList, &printLock, &printWait](
				XRuntime* rt, XObj* pContext,
				ARGS& params, KWARGS& kwParams,
				X::Value& retValue)
			{
				std::string strExeNum;
				auto it0 = kwParams.find("exe_num");
				if (it0)
				{
					strExeNum = it0->val.ToString();
				}
				std::string printInfo;
				auto it2 = kwParams.find("data");
				if (it2)
				{
					printInfo = /*"$" + strExeNum + "$" +*/ it2->val.ToString();
				}

				printLock.Lock();
				printDataList.push_back(printInfo);
				printLock.Unlock();
				printWait.Release();
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
						X::g_pXHost->RunCode("devops_run.x", code.c_str(),(int)code.size(), retVal);
						if (retVal.IsObject() && retVal.GetObj()->GetType() == ObjType::Str)
						{
							retData = retVal.ToString();
						}
						else
						{
							retData = retVal.ToString(true);
						}
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
		m_srv.Get("/devops/runcodeline",
			[this, &printWait, &bCodelineRunning](const httplib::Request& req,httplib::Response& res)
			{
				auto& req_params = req.params;
				auto itNum = req_params.find("exe_num");
				auto itCode = req_params.find("code");
				std::string retData;
				if (itNum != req_params.end() && itCode != req_params.end())
				{
					int iExeNum = std::stoi(itNum->second);
					std::string code = itCode->second;
					//std::cout << "code:\n" << code << std::endl;
					if (X::g_pXHost)
					{
						X::Value retVal;
						bCodelineRunning = true;
						printWait.Release();
						X::g_pXHost->RunCodeLine(code.c_str(), (int)code.size(), retVal, iExeNum);
						bCodelineRunning = false;
						printWait.Release();
						if (retVal.IsObject() && retVal.GetObj()->GetType() == ObjType::Str)
						{
							retData = retVal.ToString();
						}
						else
						{
							retData = retVal.ToString(true);
						}
						res.set_content(retData, "text/html");
					}
					else
						res.set_content("xlang server internal error", "text/html");
				}
				else
				{
					res.set_content("args format error", "text/html");
				}
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
					notiWait.Wait(1000);
					notiLock.Lock();
				}
				if (notis.size() > 0)
				{
					notifyInfo = notis[0];
					notis.erase(notis.begin());
				}
				else
					notifyInfo = "null";
				notiLock.Unlock();
				res.set_content(notifyInfo, "text/html");
			}
		);
		m_srv.Get("/devops/getprint",
			[this, &printDataList, &printLock, &printWait, &bCodelineRunning](const httplib::Request& req, httplib::Response& res){
				res.set_content_provider(
					"text/plain",
					[&](size_t offset, httplib::DataSink& sink) {
						printLock.Lock();
						if (printDataList.size() == 0 && bCodelineRunning) // wait print data
						{
							printLock.Unlock();
							printWait.Wait(-1);
							printLock.Lock();
						}

						if (printDataList.size() > 0) // has print data
						{
							std::string printStr = std::move(printDataList[0]);
							printDataList.erase(printDataList.begin());
							printLock.Unlock();
							sink.write(printStr.c_str(), printStr.size());
						}
						else if (!bCodelineRunning)// run end and no print data
						{
							printLock.Unlock();
							sink.done();
						}
						return true;
					}
				);
			}
		);
		m_srv.Get("/devops/checkStarted",
			[this](const httplib::Request& req, httplib::Response& res)
			{
				std::lock_guard<std::mutex> lock(m_mtxConnect);
				m_Connected = true;
				m_cvConnect.notify_one();
			}
		);
		m_srv.Get("/devops/terminate",
			[this](const httplib::Request& req, httplib::Response& res)
			{
				m_srv.stop();
				system("pause");
				std::exit(0);
			}
		);
		if (!m_srv.is_valid())
		{
			printf("Devops server has an error...\n");
		}
		std::cout << "DevServer set debug mode: true" << std::endl;
		X::g_pXHost->SetDebugMode(true);
		std::cout << "DevServer listens on port:" << m_port << std::endl;
		if (!m_srv.listen("::", m_port))
		{
			std::cout << "listen failed" << std::endl;
			std::cout << "DevServer set debug mode: false" << std::endl;
			X::g_pXHost->SetDebugMode(false);
		}
		//exit
		notiWait.Release();
		X::OffEvent("devops.dbg", dbg_handler_cookie);
		X::OffEvent("devops.print2jupyter", print_cookie);
		
	}
}
