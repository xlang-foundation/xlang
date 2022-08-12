#include "devops.h"
#include <iostream>
#include "Hosting.h"
#include "manager.h"
#include "dict.h"
#include "list.h"
#include "pyproxyobject.h"
#include "gthread.h"
#include "httplib.h"
#include <chrono>
#include "port.h"
#include "event.h"

namespace X
{
	namespace DevOps
	{
		#define	dbg_evt_name "Devops.Dbg"
		class DebuggerImpl :
			public Debugger,
			public GThread
		{
			bool m_bRun = false;
			bool m_bRegistered = false;
			httplib::Client* m_pClient = nullptr;
			std::string m_sessionId;
			inline virtual bool Init() override
			{
				REGISTER_PACKAGE("xdb", DebugService);
				X::Event* pEvt = X::EventSystem::I().Register(dbg_evt_name);
				if (pEvt)
				{
					pEvt->Release();
				}
				std::cout << "DebuggerImpl::Start()" << std::endl;
				m_bRun = true;
				return GThread::Start();
			}
			inline virtual bool Uninit() override
			{
				GThread::Stop();
				X::EventSystem::I().Unregister(dbg_evt_name);
				std::cout << "DebuggerImpl::Stop()" << std::endl;
				return true;
			}

			int64_t getCurrentMSinceEpoch()
			{
				return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
			}
			void CheckAndRegister(httplib::Client& cli)
			{
				if (m_bRegistered)
				{
					return;
				}
				auto pid = GetPID();
				//char szHostName[255];
				//gethostname(szHostName, 255);
				const int online_len = 1000;
				char regUrl[online_len];
				SPRINTF(regUrl, online_len, "/register?pid=%lu", pid);
				if (auto res = cli.Get(regUrl))
				{
					if (res->status == 200) 
					{
						m_sessionId = res->body;
						m_bRegistered = true;
						std::cout <<"Registered:"<<m_sessionId << std::endl;
					}
				}
				else 
				{
					auto err = res.error();
					std::cout << "HTTP error: " << httplib::to_string(err) << std::endl;
				}
			}
			// Inherited via GThread
			virtual void run() override
			{
				//if use localhost, will take long time to solve the name in DNS
				httplib::Client cli("http://192.168.1.246:3141");
				cli.set_keep_alive(true);
				m_pClient = &cli;
				X::Event* pEvt = X::EventSystem::I().Query(dbg_evt_name);
				void* h = pEvt->Add([](void* pContext, X::Event* pEvt) 
				{
					DebuggerImpl* pThis = (DebuggerImpl*)pContext;
					std::string sessionId = pThis->GetSessionId();
					httplib::Client* pCli = pThis->GetHttpClient();
					auto valAction = pEvt->Get("action");
					auto strAction = valAction.ToString();
					std::string notifyInfo;
					if (strAction == "end")
					{
						notifyInfo = "end";
					}
					else if (strAction == "notify")
					{
						auto valParam = pEvt->Get("param");
						auto strParam = valParam.ToString();
						notifyInfo = "$notify$" + strParam;
					}
					httplib::Headers headBack;
					headBack.emplace(std::make_pair("sessionId", sessionId));
					auto res = pCli->Post("/send_ack",
						headBack,
						notifyInfo.c_str(), notifyInfo.size(),
						"text/plain");
					std::cout << "Sent out Notify:" << notifyInfo << std::endl;
				}, this);
				while (m_bRun)
				{
					CheckAndRegister(cli);
					httplib::Headers headGet;
					headGet.emplace(std::make_pair("sessionId", m_sessionId));
					int64_t t1 = getCurrentMSinceEpoch();
					auto res = cli.Get("/get_cmd", headGet);
					int64_t t2 = getCurrentMSinceEpoch();
					//std::cout << "diff:" <<t2-t1	<< std::endl;
					auto err = res.error();
					if (err != httplib::Error::Success)
					{
						continue;
					}
					if (res->status != 200)
					{
						continue;
					}
					std::string cmdId = res->get_header_value("cmd_id");
					if (res->body == "None")
					{
						MS_SLEEP(10);
						continue;
					}
					//std::cout << res->body << std::endl;
					std::string ack = ProcessData(
						(char*)res->body.c_str(), (int)res->body.size());
					httplib::Headers headBack;
					headBack.emplace(std::make_pair("sessionId", m_sessionId));
					headBack.emplace(std::make_pair("cmd_id", cmdId));
					res = cli.Post("/send_ack", headBack,
						ack.c_str(), ack.size(),
						"text/plain");
				}
			}
			std::string ProcessData(char* data, int size)
			{
				//std::cout << data << std::endl;
				std::string moduleName("debugger.x");
				AST::Value retVal;
				std::string ack("OK");
				if (Hosting::I().Run(moduleName, data, size, retVal))
				{
					if (retVal.IsInvalid())
					{
						retVal = retVal;
					}
					ack = retVal.ToString(true);
				}
				else
				{
					ack = "Failed";
				}
				//std::cout << "back:" << ack<< std::endl;
				return ack;
			}
		public:
			DebuggerImpl() :Debugger(0)
			{
				std::cout << "DebuggerImpl()" << std::endl;
			}
			~DebuggerImpl()
			{
				std::cout << "~DebuggerImpl()" << std::endl;
			}
			httplib::Client* GetHttpClient()
			{
				return m_pClient;
			}
			std::string& GetSessionId()
			{
				return m_sessionId;
			}
		};

		Debugger::Debugger()
		{
			std::cout << "Debugger" << std::endl;
			mImpl = new DebuggerImpl();
			std::cout << "After Impl of Debugger" << std::endl;
		}
		Debugger::~Debugger()
		{
			std::cout << "~Debugger()" << std::endl;
			if (mImpl)
			{
				delete mImpl;
			}
			std::cout << "After Impl of DebuggerImpl" << std::endl;
		}
		bool DebugService::BuildLocals(Runtime* rt,
			void* pContextCurrent,int frameId,
			AST::Value& valLocals)
		{
			int index = 0;
			AST::StackFrame* pCurStack = rt->GetCurrentStack();
			while (pCurStack != nil)
			{
				if (index == frameId)
				{
					break;
				}
				pCurStack = pCurStack->Prev();
				index++;
			}
			Data::List* pList = new Data::List();
			if (pCurStack)
			{
				AST::Scope* pCurScope = pCurStack->GetScope();
				pCurScope->EachVar(rt, pContextCurrent,[rt,pList](
					std::string name, 
					AST::Value& val)
				{
					if (val.IsObject() && val.GetObj()->IsFunc())
					{
						return;
					}
					Data::Dict* dict = new Data::Dict();
					Data::Str* pStrName = new Data::Str(name);
					dict->Set("Name", AST::Value(pStrName));

					auto valType = val.GetValueType();
					Data::Str* pStrType = new Data::Str(valType);
					dict->Set("Type", AST::Value(pStrType));
					if (!val.IsObject()
						|| (val.IsObject() && val.GetObj()->IsStr()))
					{
						dict->Set("Value", val);
					}
					else if (val.IsObject())
					{
						AST::Value objId((unsigned long long)val.GetObj());
						dict->Set("Value", objId);
						AST::Value valSize(val.GetObj()->Size());
						dict->Set("Size", valSize);
					}
					AST::Value valDict(dict);
					pList->Add(rt, valDict);
				});
			}
			valLocals = AST::Value(pList);
			return true;
		}
		bool DebugService::BuildObjectContent(Runtime* rt,
			void* pContextCurrent, int frameId,AST::Value& valParam,
			AST::Value& valObject)
		{
			if (!valParam.IsObject())
			{
				return false;
			}
			//[objid,start Index,count]
			Data::List* pParamObj = dynamic_cast<Data::List*>(valParam.GetObj());
			if (pParamObj == nullptr || pParamObj->Size()==0)
			{
				return false;
			}
			AST::Value valObjReq;
			pParamObj->Get(0, valObjReq);
			Data::Object* pObjReq = (Data::Object*)valObjReq.GetLongLong();
			long long startIdx = 0;
			if (pParamObj->Size() >= 1)
			{
				AST::Value valStart;
				pParamObj->Get(1, valStart);
				startIdx = valStart.GetLongLong();
			}
			long long reqCount = -1;
			if (pParamObj->Size() >= 2)
			{
				AST::Value valCount;
				pParamObj->Get(2, valCount);
				reqCount = valCount.GetLongLong();
			}
			Data::List* pList = pObjReq->FlatPack(rt,startIdx, reqCount);
			valObject = AST::Value(pList);
			return true;
		}
		bool DebugService::BuildStackInfo(
			Runtime* rt, void* pContextCurrent,
			AST::CommandInfo* pCommandInfo,
			AST::Value& valStackInfo)
		{
			TraceEvent traceEvent = pCommandInfo->m_traceEvent;
			int index = 0;
			AST::StackFrame* pCurStack = rt->GetCurrentStack();
			Data::List* pList = new Data::List();
			while (pCurStack != nil)
			{
				int line = pCurStack->GetStartLine();
				int column = pCurStack->GetCharPos();

				AST::Scope* pMyScope = pCurStack->GetScope();
				std::string moduleFileName = pMyScope->GetModuleName(rt);
				if (pMyScope)
				{
					Data::Dict* dict = new Data::Dict();
					dict->Set("index", AST::Value(index));
					std::string name = pMyScope->GetNameString();
					if (name.empty())
					{
						AST::Func* pFunc = dynamic_cast<AST::Func*>(pMyScope);
						if (pFunc)
						{
							char v[1000];
							snprintf(v, sizeof(v), "lambda:(%d,%d)0x%llx",
								pFunc->GetStartLine(), pFunc->GetCharPos(),
								(unsigned long long)pFunc);
							name = v;
						}
					}
					Data::Str* pStrName = new Data::Str(name);
					dict->Set("name",AST::Value(pStrName));
					Data::Str* pStrFileName = new Data::Str(moduleFileName);
					dict->Set("file", AST::Value(pStrFileName));
					dict->Set("line",AST::Value(line));
					dict->Set("column",AST::Value(column));
					AST::Value valDict(dict);
					pList->Add(rt, valDict);
					index++;
				}
				pCurStack = pCurStack->Prev();
			}
			valStackInfo = AST::Value(pList);
			return true;
		}
		bool DebugService::GetModuleStartLine(void* rt, void* pContext,
			ARGS& params, KWARGS& kwParams, AST::Value& retValue)
		{
			if (params.size() == 0)
			{
				retValue = AST::Value(false);
				return true;
			}
			unsigned long long moduleKey = params[0].GetLongLong();
			AST::Module* pModule = Hosting::I().QueryModule(moduleKey);
			int nStartLine = -1;
			if (pModule)
			{
				nStartLine = pModule->GetStartLine();
			}
			retValue = AST::Value(nStartLine);
			return true;
		}
		bool DebugService::SetBreakpoints(void* rt, void* pContext,
			ARGS& params, KWARGS& kwParams, AST::Value& retValue)
		{
			if (params.size() != 2)
			{
				retValue = AST::Value(false);
				return false;
			}
			unsigned long long moduleKey = params[0].GetLongLong();
			AST::Value varLines = params[1];
			AST::Module* pModule = Hosting::I().QueryModule(moduleKey);
			if (pModule == nullptr)
			{
				retValue = varLines;
				return true;
			}
			if (!varLines.IsObject()
				|| varLines.GetObj()->GetType() != X::Data::Type::List)
			{
				retValue = AST::Value(false);
				return true;
			}
			auto* pLineList = dynamic_cast<X::Data::List*>(varLines.GetObj());
			auto lines = pLineList->Map<int>(
				[](AST::Value& elm, unsigned long long idx) {
				return elm;}
			);
			pModule->ClearBreakpoints();
			Data::List* pList = new Data::List();
			for (auto l : lines)
			{
				l = pModule->SetBreakpoint(l,(int)GetThreadID());
				if (l >= 0)
				{
					AST::Value varL(l);
					pList->Add((Runtime*)rt,varL);
				}
			}
			retValue = AST::Value(pList);
			return true;
		}
		bool DebugService::Command(void* rt, void* pContext,
			ARGS& params, KWARGS& kwParams, AST::Value& retValue)
		{
			if (params.size() == 0)
			{
				retValue = AST::Value(false);
				return true;
			}
			unsigned long long moduleKey = params[0].GetLongLong();
			AST::Module* pModule = Hosting::I().QueryModule(moduleKey);
			if (pModule == nullptr)
			{
				retValue = AST::Value(false);
				return true;
			}
			std::string strCmd;
			auto it = kwParams.find("cmd");
			if (it != kwParams.end())
			{
				strCmd = it->second.ToString();
			}
			AST::Value valParam;
			it = kwParams.find("param");
			if (it != kwParams.end())
			{
				valParam = it->second;
			}
			AST::CommandInfo cmdInfo;
			if (strCmd == "Stack")
			{
				auto stackTracePack = [](Runtime* rt,
					void* pContextCurrent,
					AST::CommandInfo* pCommandInfo,
					AST::Value& retVal)
				{
					DebugService* pDebugService = (DebugService*)
						pCommandInfo->m_callContext;
					pDebugService->BuildStackInfo(rt, pContextCurrent,
						pCommandInfo, retVal);
				};
				cmdInfo.m_callContext = this;
				cmdInfo.m_process = stackTracePack;
				cmdInfo.m_retValueHolder = &retValue;
				cmdInfo.dbgType = AST::dbg::StackTrace;
				pModule->AddCommand(cmdInfo, true);
			}
			else if (strCmd == "Locals" || strCmd=="Object")
			{
				int frameId = 0;
				auto it2 = kwParams.find("frameId");
				if (it2 != kwParams.end())
				{
					frameId = (int)it2->second.GetLongLong();
				}
				cmdInfo.m_frameId = frameId;
				cmdInfo.dbgType = AST::dbg::GetRuntime;
				auto localPack = [](Runtime* rt,
					void* pContextCurrent,
					AST::CommandInfo* pCommandInfo,
					AST::Value& retVal)
				{
					DebugService* pDebugService = (DebugService*)
						pCommandInfo->m_callContext;
					pDebugService->BuildLocals(rt, pContextCurrent,
						pCommandInfo->m_frameId, retVal);
				};
				auto objPack = [](Runtime* rt,
					void* pContextCurrent,
					AST::CommandInfo* pCommandInfo,
					AST::Value& retVal)
				{
					DebugService* pDebugService = (DebugService*)
						pCommandInfo->m_callContext;
					pDebugService->BuildObjectContent(rt, pContextCurrent,
						pCommandInfo->m_frameId,
						pCommandInfo->m_varParam,
						retVal);
				};
				if (strCmd == "Locals")
				{
					cmdInfo.m_process = localPack;
				}
				else if (strCmd == "Object")
				{
					cmdInfo.m_process = objPack;
				}
				cmdInfo.m_varParam = valParam;
				cmdInfo.m_callContext = this;
				cmdInfo.m_retValueHolder = &retValue;
				pModule->AddCommand(cmdInfo, true);
			}
			if (strCmd == "Step")
			{
				cmdInfo.dbgType = AST::dbg::Step;
				pModule->AddCommand(cmdInfo, false);
				retValue = AST::Value(true);
			}
			else if (strCmd == "Continue")
			{
				cmdInfo.dbgType = AST::dbg::Continue;
				pModule->AddCommand(cmdInfo, false);
				retValue = AST::Value(true);
			}
			else if (strCmd == "StepIn")
			{
				cmdInfo.dbgType = AST::dbg::StepIn;
				pModule->AddCommand(cmdInfo, false);
				retValue = AST::Value(true);
			}
			else if (strCmd == "StepOut")
			{
				cmdInfo.dbgType = AST::dbg::StepOut;
				pModule->AddCommand(cmdInfo, false);
				retValue = AST::Value(true);
			}
			return true;
		}
	}
}

