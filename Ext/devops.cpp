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
			X::ObjectEvent* pEvt = X::EventSystem::I().Register(dbg_evt_name);
			if (pEvt)
			{
				pEvt->IncRef();
			}
		}
		bool DebugService::BuildLocals(XlangRuntime* rt,
			XObj* pContextCurrent,int frameId,
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
					else if (val.IsObject() && 
						val.GetObj()->GetType() == X::ObjType::Function)
					{
						auto* pFuncObj = dynamic_cast<X::Data::Function*>(val.GetObj());
						std::string strDoc = pFuncObj->GetDoc();
						val = strDoc;
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
		bool DebugService::BuildGlobals(XlangRuntime* rt,
			XObj* pContextCurrent,
			X::Value& valGlobals)
		{
			Data::List* pList = new Data::List();
			AST::Scope* pCurScope = rt->M();
			pCurScope->EachVar(rt, pContextCurrent, [rt, pList](
				std::string name,
				X::Value& val)
				{
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
					else if (val.IsObject() &&
						val.GetObj()->GetType() == X::ObjType::Function)
					{
						auto* pFuncObj = dynamic_cast<X::Data::Function*>(val.GetObj());
						std::string strDoc = pFuncObj->GetDoc();
						val = strDoc;
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
			valGlobals = X::Value(pList);
			return true;
		}
		bool DebugService::BuildObjectContent(XlangRuntime* rt,
			XObj* pContextCurrent, int frameId,X::Value& valParam,
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
			X::Value valContext;
			pParamObj->Get(1, valContext);
			X::XObj* pContextObj = (X::XObj*)(valContext.GetLongLong());
			Data::Object* pObjReq = dynamic_cast<Data::Object*>((XObj*)valObjReq.GetLongLong());
			long long startIdx = 0;
			if (pParamObj->Size() >= 2)
			{
				X::Value valStart;
				pParamObj->Get(2, valStart);
				startIdx = valStart.GetLongLong();
			}
			long long reqCount = -1;
			if (pParamObj->Size() >= 3)
			{
				X::Value valCount;
				pParamObj->Get(3, valCount);
				reqCount = valCount.GetLongLong();
			}
			Data::List* pList = pObjReq->FlatPack(rt, pContextObj,startIdx, reqCount);
			//pList already hold one refcount when return from FlatPack
			//so don't need X::Value to add refcount
			valObject = X::Value(pList,false);
			return true;
		}
		bool DebugService::BuildStackInfo(
			XlangRuntime* rt, XObj* pContextCurrent,
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
		int DebugService::GetModuleStartLine(unsigned long long moduleKey)
		{
			AST::Module* pModule = Hosting::I().QueryModule(moduleKey);
			int nStartLine = -1;
			if (pModule)
			{
				nStartLine = pModule->GetStartLine();
			}
			return nStartLine;
		}
		X::Value DebugService::SetBreakpoints(X::XRuntime* rt, X::XObj* pContext,
			unsigned long long moduleKey, Value& varLines)
		{
			AST::Module* pModule = Hosting::I().QueryModule(moduleKey);
			if (pModule == nullptr)
			{
				return varLines;
			}
			if (!varLines.IsObject()
				|| varLines.GetObj()->GetType() != X::ObjType::List)
			{
				return X::Value(false);
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
					pList->Add((XlangRuntime*)rt,varL);
				}
			}
			return X::Value(pList);
		}
		bool DebugService::Command(X::XRuntime* rt, XObj* pContext,
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
			if (strCmd == "Stack")
			{
				auto stackTracePack = [](XlangRuntime* rt,
					XObj* pContextCurrent,
					AST::CommandInfo* pCommandInfo,
					X::Value& retVal)
				{
					DebugService* pDebugService = (DebugService*)
						pCommandInfo->m_callContext;
					pDebugService->BuildStackInfo(rt, pContextCurrent,
						pCommandInfo, retVal);
				};
				AST::CommandInfo* pCmdInfo = new AST::CommandInfo();
				pCmdInfo->m_callContext = this;
				pCmdInfo->m_process = stackTracePack;
				pCmdInfo->m_needRetValue = true;
				pCmdInfo->dbgType = AST::dbg::StackTrace;
				pModule->AddCommand(pCmdInfo, true);
				retValue = pCmdInfo->m_retValueHolder;
				delete pCmdInfo;
			}
			else if (strCmd == "Globals" 
					|| strCmd == "Locals" 
					|| strCmd=="Object")
			{
				int frameId = 0;
				auto it2 = kwParams.find("frameId");
				if (it2 != kwParams.end())
				{
					frameId = (int)it2->second.GetLongLong();
				}
				AST::CommandInfo* pCmdInfo = new AST::CommandInfo();
				pCmdInfo->m_frameId = frameId;
				pCmdInfo->dbgType = AST::dbg::GetRuntime;
				auto globalPack = [](XlangRuntime* rt,
					XObj* pContextCurrent,
					AST::CommandInfo* pCommandInfo,
					X::Value& retVal)
				{
					DebugService* pDebugService = (DebugService*)
						pCommandInfo->m_callContext;
					pDebugService->BuildGlobals(rt, pContextCurrent,retVal);
				};
				auto localPack = [](XlangRuntime* rt,
					XObj* pContextCurrent,
					AST::CommandInfo* pCommandInfo,
					X::Value& retVal)
				{
					DebugService* pDebugService = (DebugService*)
						pCommandInfo->m_callContext;
					pDebugService->BuildLocals(rt, pContextCurrent,
						pCommandInfo->m_frameId, retVal);
				};
				auto objPack = [](XlangRuntime* rt,
					XObj* pContextCurrent,
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
					pCmdInfo->m_process = localPack;
				}
				if (strCmd == "Globals")
				{
					pCmdInfo->m_process = globalPack;
				}
				else if (strCmd == "Object")
				{
					pCmdInfo->m_process = objPack;
				}
				pCmdInfo->m_varParam = valParam;
				pCmdInfo->m_callContext = this;
				pCmdInfo->m_needRetValue =true;
				pModule->AddCommand(pCmdInfo, true);
				retValue = pCmdInfo->m_retValueHolder;
				delete pCmdInfo;
			}
			if (strCmd == "Step")
			{
				AST::CommandInfo* pCmdInfo = new AST::CommandInfo();
				pCmdInfo->m_downstreamDelete = true;
				pCmdInfo->dbgType = AST::dbg::Step;
				pModule->AddCommand(pCmdInfo, false);
				retValue = X::Value(true);
			}
			else if (strCmd == "Continue")
			{
				AST::CommandInfo* pCmdInfo = new AST::CommandInfo();
				pCmdInfo->m_downstreamDelete = true;
				pCmdInfo->dbgType = AST::dbg::Continue;
				pModule->AddCommand(pCmdInfo, false);
				retValue = X::Value(true);
			}
			else if (strCmd == "StepIn")
			{
				AST::CommandInfo* pCmdInfo = new AST::CommandInfo();
				pCmdInfo->m_downstreamDelete = true;
				pCmdInfo->dbgType = AST::dbg::StepIn;
				pModule->AddCommand(pCmdInfo, false);
				retValue = X::Value(true);
			}
			else if (strCmd == "StepOut")
			{
				AST::CommandInfo* pCmdInfo = new AST::CommandInfo();
				pCmdInfo->m_downstreamDelete = true;
				pCmdInfo->dbgType = AST::dbg::StepOut;
				pModule->AddCommand(pCmdInfo, false);
				retValue = X::Value(true);
			}
			return true;
		}
	}
}

