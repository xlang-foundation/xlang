/*
Copyright (C) 2024 The XLang Foundation
Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

#include "devsrv.h"
#include "Locker.h"
#include "wait.h"
#include <iostream>
#include <atomic>
#include <thread>
#include <chrono>
#include <filesystem>
#include "xlog.h"

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
#if (WIN32)
					system("pause");
#endif
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
		std::atomic_bool bFragmentCodeRunning = true;

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
		m_srv.Post("/devops/run",
			[this](const httplib::Request& req,httplib::Response& res)
			{
				std::string retData("error");
				if (req.body.size() > 4 && X::g_pXHost)
				{
					std::string code, src, md5;
					const char* data = req.body.data();
					uint32_t codeLen = ntohl(*reinterpret_cast<const uint32_t*>(data));
					data += 4;
					code = std::string(data, codeLen);
					if (req.body.size() > codeLen + 4)
					{
						data += codeLen;
						uint32_t srcLen = ntohl(*reinterpret_cast<const uint32_t*>(data));
						data += 4;
						src = std::string(data, srcLen);
						data += srcLen;
						uint32_t md5Len = ntohl(*reinterpret_cast<const uint32_t*>(data));
						data += 4;
						md5 = std::string(data, md5Len);
						X::Value v(src);
						X::g_pXHost->SetAttr(X::Value(), md5.c_str(), v);
					}
					//std::cout << "code:\n" << code << std::endl;
					X::Value retVal;
					X::g_pXHost->RunCodeInNonDebug("devops_run.x", code.c_str(), (int)code.size(), retVal);
					if (retVal.IsObject() && retVal.GetObj()->GetType() == ObjType::Str)
					{
						retData = retVal.ToString();
					}
					else
					{
						retData = retVal.ToString(true);
					}
				}
				res.set_content(retData, "text/html");
				//std::cout << "BackData:" << retData << std::endl;
			}
		);
		m_srv.Post("/devops/runfragmentcode",
			[this, &printWait, &bFragmentCodeRunning](const httplib::Request& req,httplib::Response& res)
			{
				if (m_JupyterModule.IsInvalid())
				{
					m_JupyterModule = X::g_pXHost->NewModule();
				}
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
						bFragmentCodeRunning.store(true);
						printWait.Release();
						X::g_pXHost->RunFragmentInModule(m_JupyterModule, code.c_str(), (int)code.size(), retVal, iExeNum);
						bFragmentCodeRunning.store(false);
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
			[this, &printDataList, &printLock, &printWait, &bFragmentCodeRunning](const httplib::Request& req, httplib::Response& res){
				res.set_content_provider(
					"text/plain",
					[&](size_t offset, httplib::DataSink& sink) {
						printLock.Lock();
						if (printDataList.size() == 0 && bFragmentCodeRunning.load()) // wait print data
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
						else if (!bFragmentCodeRunning.load())// run end and no print data
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

				// check if debug need input a path
				auto& req_params = req.params;
				auto itPath = req_params.find("path");
				auto itPlatform = req_params.find("platform");
				auto itMd5 = req_params.find("md5");

				if (itPath != req_params.end() && itPlatform != req_params.end())
				{
					std::string path = itPath->second;
					std::string platform = itPlatform->second;
					std::string md5 = itMd5->second;
					if (X::g_pXHost->IsModuleLoadedMd5(md5.c_str()))
						res.set_content("not_need_path", "text/html");
#if defined(_WIN32) 
					else if (platform == "not_windows") // platfor not match
						res.set_content("need_path", "text/html");
#else 
					else if (platform == "windows") // platfor not match
						res.set_content("need_path", "text/html"); 
#endif
					else
					{
						if (std::filesystem::exists(path))
							res.set_content("not_need_path", "text/html");
						else // file to debug not exist
							res.set_content("need_path", "text/html");
					}
				}
				LOG << LOG_GREEN << "xlang debug client connected" << LOG_RESET << LINE_END;
			}
		);
		m_srv.Get("/devops/terminate",
			[this](const httplib::Request& req, httplib::Response& res)
			{
				m_srv.stop();
				std::exit(0);
			}
		);
		if (!m_srv.is_valid())
		{
			LOG << LOG_RED << "Devops server has an error..." << LOG_RESET << LINE_END;
		}
		LOG << LOG_GREEN << "DevServer set debug mode: true" << LOG_RESET << LINE_END;
		X::g_pXHost->SetDebugMode(true);
		LOG << LOG_GREEN << "DevServer listens on port:" << m_port << LOG_RESET << LINE_END;
		if (!m_srv.listen("::", m_port))
		{
			LOG << LOG_RED << "listen failed" << LOG_RESET << LINE_END;
			LOG <<LOG_RED << "DevServer set debug mode: false" << LOG_RESET <<LINE_END;
			X::g_pXHost->SetDebugMode(false);
		}
		//exit
		notiWait.Release();
		X::OffEvent("devops.dbg", dbg_handler_cookie);
		X::OffEvent("devops.print2jupyter", print_cookie);
		
	}
}
