#include "devops.h"
#include <iostream>
#include "Hosting.h"
#include "manager.h"
#include "dict.h"
#include "list.h"
#include "pyproxyobject.h"
#include "gthread.h"
#include <chrono>
#include "port.h"
#include "event.h"

namespace X
{
	namespace DevOps
	{
		#define	dbg_evt_name "devops.dbg"
		DebugService::DebugService()
		{
			X::Event* pEvt = X::EventSystem::I().Register(dbg_evt_name);
			if (pEvt)
			{
				pEvt->IncRef();
			}
		}
		bool DebugService::BuildLocals(Runtime* rt,
			void* pContextCurrent,int frameId,
			X::Value& valLocals)
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
					X::Value& val)
				{
					if (val.IsObject() && 
						dynamic_cast<Data::Object*>(val.GetObj())->IsFunc())
					{
						return;
					}
					Data::Dict* dict = new Data::Dict();
					Data::Str* pStrName = new Data::Str(name);
					dict->Set("Name", X::Value(pStrName));

					auto valType = val.GetValueType();
					Data::Str* pStrType = new Data::Str(valType);
					dict->Set("Type", X::Value(pStrType));
					if (!val.IsObject()
						|| (val.IsObject() && 
							dynamic_cast<Data::Object*>(val.GetObj())->IsStr()))
					{
						dict->Set("Value", val);
					}
					else if (val.IsObject())
					{
						X::Value objId((unsigned long long)val.GetObj());
						dict->Set("Value", objId);
						X::Value valSize(val.GetObj()->Size());
						dict->Set("Size", valSize);
					}
					X::Value valDict(dict);
					pList->Add(rt, valDict);
				});
			}
			valLocals = X::Value(pList);
			return true;
		}
		bool DebugService::BuildObjectContent(Runtime* rt,
			void* pContextCurrent, int frameId,X::Value& valParam,
			X::Value& valObject)
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
			X::Value valObjReq;
			pParamObj->Get(0, valObjReq);
			Data::Object* pObjReq = dynamic_cast<Data::Object*>((XObj*)valObjReq.GetLongLong());
			long long startIdx = 0;
			if (pParamObj->Size() >= 1)
			{
				X::Value valStart;
				pParamObj->Get(1, valStart);
				startIdx = valStart.GetLongLong();
			}
			long long reqCount = -1;
			if (pParamObj->Size() >= 2)
			{
				X::Value valCount;
				pParamObj->Get(2, valCount);
				reqCount = valCount.GetLongLong();
			}
			Data::List* pList = pObjReq->FlatPack(rt,startIdx, reqCount);
			valObject = X::Value(pList);
			return true;
		}
		bool DebugService::BuildStackInfo(
			Runtime* rt, void* pContextCurrent,
			AST::CommandInfo* pCommandInfo,
			X::Value& valStackInfo)
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
					dict->Set("index", X::Value(index));
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
					dict->Set("name",X::Value(pStrName));
					Data::Str* pStrFileName = new Data::Str(moduleFileName);
					dict->Set("file", X::Value(pStrFileName));
					dict->Set("line",X::Value(line));
					dict->Set("column",X::Value(column));
					X::Value valDict(dict);
					pList->Add(rt, valDict);
					index++;
				}
				pCurStack = pCurStack->Prev();
			}
			valStackInfo = X::Value(pList);
			return true;
		}
		bool DebugService::GetModuleStartLine(void* rt, void* pContext,
			ARGS& params, KWARGS& kwParams, X::Value& retValue)
		{
			if (params.size() == 0)
			{
				retValue = X::Value(false);
				return true;
			}
			unsigned long long moduleKey = params[0].GetLongLong();
			AST::Module* pModule = Hosting::I().QueryModule(moduleKey);
			int nStartLine = -1;
			if (pModule)
			{
				nStartLine = pModule->GetStartLine();
			}
			retValue = X::Value(nStartLine);
			return true;
		}
		bool DebugService::SetBreakpoints(void* rt, void* pContext,
			ARGS& params, KWARGS& kwParams, X::Value& retValue)
		{
			if (params.size() != 2)
			{
				retValue = X::Value(false);
				return false;
			}
			unsigned long long moduleKey = params[0].GetLongLong();
			X::Value varLines = params[1];
			AST::Module* pModule = Hosting::I().QueryModule(moduleKey);
			if (pModule == nullptr)
			{
				retValue = varLines;
				return true;
			}
			if (!varLines.IsObject()
				|| varLines.GetObj()->GetType() != X::ObjType::List)
			{
				retValue = X::Value(false);
				return true;
			}
			auto* pLineList = dynamic_cast<X::Data::List*>(varLines.GetObj());
			auto lines = pLineList->Map<int>(
				[](X::Value& elm, unsigned long long idx) {
				return elm;}
			);
			pModule->ClearBreakpoints();
			Data::List* pList = new Data::List();
			for (auto l : lines)
			{
				l = pModule->SetBreakpoint(l,(int)GetThreadID());
				if (l >= 0)
				{
					X::Value varL(l);
					pList->Add((Runtime*)rt,varL);
				}
			}
			retValue = X::Value(pList);
			return true;
		}
		bool DebugService::Command(void* rt, void* pContext,
			ARGS& params, KWARGS& kwParams, X::Value& retValue)
		{
			if (params.size() == 0)
			{
				retValue = X::Value(false);
				return true;
			}
			unsigned long long moduleKey = params[0].GetLongLong();
			AST::Module* pModule = Hosting::I().QueryModule(moduleKey);
			if (pModule == nullptr)
			{
				retValue = X::Value(false);
				return true;
			}
			std::string strCmd;
			auto it = kwParams.find("cmd");
			if (it != kwParams.end())
			{
				strCmd = it->second.ToString();
			}
			X::Value valParam;
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
					X::Value& retVal)
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
					X::Value& retVal)
				{
					DebugService* pDebugService = (DebugService*)
						pCommandInfo->m_callContext;
					pDebugService->BuildLocals(rt, pContextCurrent,
						pCommandInfo->m_frameId, retVal);
				};
				auto objPack = [](Runtime* rt,
					void* pContextCurrent,
					AST::CommandInfo* pCommandInfo,
					X::Value& retVal)
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
				retValue = X::Value(true);
			}
			else if (strCmd == "Continue")
			{
				cmdInfo.dbgType = AST::dbg::Continue;
				pModule->AddCommand(cmdInfo, false);
				retValue = X::Value(true);
			}
			else if (strCmd == "StepIn")
			{
				cmdInfo.dbgType = AST::dbg::StepIn;
				pModule->AddCommand(cmdInfo, false);
				retValue = X::Value(true);
			}
			else if (strCmd == "StepOut")
			{
				cmdInfo.dbgType = AST::dbg::StepOut;
				pModule->AddCommand(cmdInfo, false);
				retValue = X::Value(true);
			}
			return true;
		}
	}
}

