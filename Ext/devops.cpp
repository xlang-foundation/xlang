#include "devops.h"
#include <iostream>
#include "ipc.h"
#include "Hosting.h"
#include "manager.h"
#include "dict.h"
#include "list.h"

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
				std::cout << data << std::endl;
				std::string moduleName("debugger.x");
				AST::Value retVal;
				std::string ack("OK");
				if (Hosting::I().Run(moduleName, data, size, retVal))
				{
					ack = retVal.ToString(true);
				}
				else
				{
					ack = "Failed";
				}
				pSession->Send((char*)ack.c_str(), ack.size());
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
		bool DebugService::BuildStackInfo(Runtime* rt,AST::Expression* pCurExp,
			AST::Value& valStackInfo)
		{
			int index = 0;
			AST::StackFrame* pCurStack = rt->GetCurrentStack();
			Data::List* pList = new Data::List();
			while (pCurStack != nil)
			{
				AST::Expression* pa = pCurStack->GetCurrentExpr();
				AST::Scope* pMyScope = pa->GetScope();
				if (pMyScope)
				{
					Data::Dict* dict = new Data::Dict();
					dict->Set("index", AST::Value(index));
					std::string name;
					AST::Func* pFunc = dynamic_cast<AST::Func*>(pMyScope);
					if (pFunc)
					{
						name = pFunc->GetNameString();
						if (name.empty())
						{
							char v[1000];
							snprintf(v, sizeof(v), "lambda:(%d,%d)0x%llx",
								pFunc->GetStartLine(),pFunc->GetCharPos(),
								(unsigned long long)pFunc);
							name = v;
						}
					}
					Data::Str* pStrName = new Data::Str(name);
					dict->Set("name",AST::Value(pStrName));
					int line = pa->GetStartLine();
					dict->Set("line",AST::Value(line));
					int column = pa->GetCharPos();
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
			if (strCmd == "Step")
			{
				cmdInfo.dbgType = AST::dbg::Step;
				AST::Expression* pExpToRun = nullptr;
				cmdInfo.m_valPlaceholder = (void**)& pExpToRun;
				pModule->AddCommand(cmdInfo,true);
				int lineToRun = -1;
				if (pExpToRun)
				{
					lineToRun = pExpToRun->GetStartLine();
				}
				retValue = AST::Value(lineToRun);
			}
			else if (strCmd == "Stack")
			{
				cmdInfo.dbgType = AST::dbg::StackTrace;
				AST::Expression* pExpToRun = nullptr;
				Runtime* pCurrentRt = nullptr;
				cmdInfo.m_valPlaceholder = (void**)&pExpToRun;
				cmdInfo.m_valPlaceholder2 = (void**)&pCurrentRt;
				pModule->AddCommand(cmdInfo, true);
				if (pExpToRun)
				{
					BuildStackInfo(pCurrentRt,pExpToRun, retValue);
				}
				else
				{
					retValue = AST::Value();
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
			else if (strCmd == "Continue")
			{

			}
			else if (strCmd == "StepIn")
			{

			}
			else if (strCmd == "StepOut")
			{

			}
			return true;
		}
	}
}

