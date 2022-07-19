#include "devops.h"
#include <iostream>
#include "ipc.h"
#include "Hosting.h"
#include "manager.h"
#include "dict.h"
#include "list.h"
#include "pyproxyobject.h"

namespace X
{
	namespace DevOps
	{
#define DevOps_Pipe_Name "\\\\.\\pipe\\x.devops"
		class DebuggerImpl :
			public Debugger
		{
			IpcServer m_ipcSrv;
			inline virtual bool Start() override
			{
				REGISTER_PACKAGE("xdb", DebugService);
				m_ipcSrv.SetBufferSize(1024 * 32);
				std::cout << "DebuggerImpl::Start()" << std::endl;
				m_ipcSrv.Start();
				return true;
			}
			inline virtual bool Stop() override
			{
				std::cout << "DebuggerImpl::Stop()" << std::endl;
				m_ipcSrv.Stop();
				return true;
			}
			void OnNewSession(IpcSession* newSession)
			{
				newSession->SetDataHandler(this,
					[](void* pContext, IpcSession* pSession,
						char* data, int size) {
							((DebuggerImpl*)pContext)->OnData(pSession, data, size);
					});
			}
			void OnData(IpcSession* pSession, char* data, int size)
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
				pSession->Send((char*)ack.c_str(), (int)ack.size());
				//std::cout << "back:" << ack<< std::endl;
			}
		public:
			DebuggerImpl() :Debugger(0),
				m_ipcSrv(DevOps_Pipe_Name, this,
					[](void* pContext, IpcSession* newSession) 
					{
						((DebuggerImpl*)pContext)->OnNewSession(newSession);
					})
			{
				std::cout << "DebuggerImpl()" << std::endl;
			}
			~DebuggerImpl()
			{
				std::cout << "~DebuggerImpl()" << std::endl;
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
				auto nameMap = pCurScope->GetVarMap();
				for (auto& it : nameMap)
				{
					int idx = it.second;
					AST::Value val;
					pCurScope->Get(rt, pContextCurrent, idx, val);
					if (val.IsInvalid())
					{//not set
						continue;
					}
					if (val.IsObject() && val.GetObj()->IsFunc())
					{
						continue;
					}
					Data::Dict* dict = new Data::Dict();
					Data::Str* pStrName = new Data::Str(it.first);
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
				}
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
			TraceEvent traceEvent,
			Runtime* rt,
			AST::Expression* pCurExp,
			AST::Value& valStackInfo)
		{
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
			AST::Module* pModule = Hosting::I().QueryModule(moduleKey);
			if (pModule == nullptr)
			{
				retValue = AST::Value(false);
				return true;
			}
			AST::Value varLines = params[1];
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
			TraceEvent traceEvent = TraceEvent::None;
			AST::CommandInfo cmdInfo;
			cmdInfo.m_traceEventPtr = &traceEvent;
			if (strCmd == "Stack")
			{
				cmdInfo.dbgType = AST::dbg::StackTrace;
				AST::Expression* pExpToRun = nullptr;
				Runtime* pCurrentRt = nullptr;
				cmdInfo.m_valPlaceholder = (void**)&pExpToRun;
				cmdInfo.m_valPlaceholder2 = (void**)&pCurrentRt;
				pModule->AddCommand(cmdInfo, true);
				if (pExpToRun)
				{
					BuildStackInfo(traceEvent,pCurrentRt,
						(AST::Expression*)pExpToRun,
						retValue);
				}
				else
				{
					retValue = AST::Value(AST::ValueType::None);
				}
			}
			else if (strCmd == "Locals" || strCmd=="Object")
			{
				int frameId = 0;
				auto it2 = kwParams.find("frameId");
				if (it2 != kwParams.end())
				{
					frameId = (int)it2->second.GetLongLong();
				}
				cmdInfo.dbgType = AST::dbg::GetRuntime;
				Runtime* pCurrentRt = nullptr;
				void* pContextCurrent = nullptr;
				cmdInfo.m_valPlaceholder = nullptr;
				cmdInfo.m_valPlaceholder2 = (void**)&pCurrentRt;
				cmdInfo.m_valPlaceholder3 = (void**)&pContextCurrent;
				pModule->AddCommand(cmdInfo, true);
				if (pCurrentRt)
				{
					if (strCmd == "Locals")
					{
						BuildLocals(pCurrentRt, pContextCurrent,
							frameId, retValue);
					}
					else if (strCmd == "Object")
					{
						BuildObjectContent(pCurrentRt, pContextCurrent,
							frameId, valParam, retValue);
					}
				}
				else
				{
					retValue = AST::Value();
				}
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

