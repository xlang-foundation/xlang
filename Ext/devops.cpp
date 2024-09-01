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
#include "utility.h"
#include "dbg.h"

namespace X
{
	namespace DevOps
	{
#define	dbg_evt_name "devops.dbg"
#define dbg_scope_special_type "Scope.Special"
		DebugService::DebugService()
		{
			X::ObjectEvent* pEvt = X::EventSystem::I().Register(dbg_evt_name);
			if (pEvt)
			{
				pEvt->IncRef();
			}
		}

		bool DebugService::BuildLocals(XlangRuntime* rt,
			XObj* pContextCurrent, AST::StackFrame* frameId,
			X::Value& valLocals)
		{
			AST::StackFrame* pCurStack = rt->GetCurrentStack();
			while (pCurStack != nil)
			{
				if (pCurStack == frameId)
				{
					break;
				}
				pCurStack = pCurStack->Prev();
			}
			bool bOK = false;
			if (pCurStack)
			{
				AST::Scope* pCurScope = pCurStack->GetScope();
				bOK = PackScopeVars(rt, pContextCurrent, pCurScope, valLocals);
			}
			return bOK;
		}
		bool DebugService::BuildGlobals(XlangRuntime* rt,
			XObj* pContextCurrent,
			X::Value& valGlobals)
		{
			AST::Scope* pCurScope = rt->M()->GetMyScope();
			return PackScopeVars(rt, pContextCurrent, pCurScope, valGlobals);
		}
		bool PackValueAsDict(Data::Dict* dict,std::string& name, X::Value& val)
		{
			Data::Str* pStrName = new Data::Str(name);
			dict->Set("Name", X::Value(pStrName));
			auto valType = val.GetValueType();
			Data::Str* pStrType = new Data::Str(valType);
			dict->Set("Type", X::Value(pStrType));
			if (!val.IsObject() || (val.IsObject() &&
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
				unsigned long long llId = (unsigned long long)val.GetObj();
				const int buf_len = 1000;
				char strBuf[buf_len];
				SPRINTF(strBuf, buf_len, "%llu", llId);
				std::string strID(strBuf);
				X::Value objId(strID);
				dict->Set("Id", objId);
				X::Value valSize(val.GetObj()->Size());
				dict->Set("Size", valSize);
			}
			return true;
		}
		/*
			output a list of dict
			{
				"Name": var name ->str
				"Type": Value type->str
				"Id": string with format  curObjId.parent.....
				"Value": value
				"Size": if it is not single value, such as list,dict etc

			}
		*/
		bool DebugService::PackScopeVars(XlangRuntime* rt,
			XObj* pContextCurrent, AST::Scope* pScope,X::Value& varPackList)
		{
			Data::List* pList = new Data::List();
			int nBuiltinFuncs = 0;
			pScope->EachVar(rt, pContextCurrent, [rt, pList, &nBuiltinFuncs](
				std::string name,
				X::Value& val)
				{
					if (val.IsObject())
					{
						auto* pObjVal = val.GetObj();
						if (pObjVal->GetType() == ObjType::Function)
						{
							auto* pDataFunc = dynamic_cast<Data::Function*>(pObjVal);
							if (pDataFunc)
							{
								auto* pAstFunc = pDataFunc->GetFunc();
								if (pAstFunc && pAstFunc->m_type == AST::ObType::BuiltinFunc)
								{
									nBuiltinFuncs++;
									return;
								}
							}
						}
					}
					Data::Dict* dict = new Data::Dict();
					bool bOK = PackValueAsDict(dict,name, val);
					if (bOK)
					{
						X::Value valDict = dict;
						pList->Add(rt, valDict);
					}
				});
			if (nBuiltinFuncs>0)
			{
				Data::Dict* dict = new Data::Dict();
				Data::Str* pStrName = new Data::Str("builtins");
				dict->Set("Name", X::Value(pStrName));
				Data::Str* pStrType = new Data::Str(dbg_scope_special_type);
				dict->Set("Type", X::Value(pStrType));
				unsigned long long llId = (unsigned long long)pScope;
				const int buf_len = 1000;
				char strBuf[buf_len];
				SPRINTF(strBuf, buf_len, "%llu", llId);
				std::string strID(strBuf);
				X::Value objId(strID);
				dict->Set("Id", objId);
				X::Value valSize(nBuiltinFuncs);
				dict->Set("Size", valSize);
				X::Value valDict = dict;
				pList->Insert(0,rt, valDict);
			}
			varPackList = X::Value(pList);
			return true;
		}
		bool DebugService::PackScopeSpecialVars(XlangRuntime* rt,
			XObj* pContextCurrent, AST::Scope* pScope, X::Value& varPackList)
		{
			Data::List* pList = new Data::List();
			pScope->EachVar(rt, pContextCurrent, [rt, pList](
				std::string name,
				X::Value& val)
				{
					if (val.IsObject())
					{
						auto* pObjVal = val.GetObj();
						if (pObjVal->GetType() == ObjType::Function)
						{
							auto* pDataFunc = dynamic_cast<Data::Function*>(pObjVal);
							if (pDataFunc)
							{
								auto* pAstFunc = pDataFunc->GetFunc();
								if (pAstFunc && pAstFunc->m_type == AST::ObType::BuiltinFunc)
								{
									Data::Dict* dict = new Data::Dict();
									bool bOK = PackValueAsDict(dict, name, val);
									if (bOK)
									{
										X::Value valDict = dict;
										pList->Add(rt, valDict);
									}
								}
							}
						}
					}
				});
			varPackList = X::Value(pList);
			return true;
		}
		bool DebugService::ObjectSetValue(XlangRuntime* rt,
			XObj* pContextCurrent, AST::StackFrame* frameId, X::Value& valParam,
			X::Value& objRetValue)
		{
			if (!valParam.IsObject())
			{
				return false;
			}
			//[objid,start Index,count]
			Data::List* pParamObj = dynamic_cast<Data::List*>(valParam.GetObj());
			if (pParamObj == nullptr || pParamObj->Size() == 0)
			{
				return false;
			}
			X::Value val0;
			pParamObj->Get(0, val0);
			std::string objIds = val0;
			pParamObj->Get(1, val0);
			std::string objType = val0;
			pParamObj->Get(2, val0);
			std::string objName = val0;
			X::Value newVal;
			pParamObj->Get(3, newVal);
			X::Value  correctedValue = newVal;
			Data::Dict* retDict = new Data::Dict();
			retDict->Set("Id", objIds);
			//if null objId, means this var is just a value
			//it is not a object, because all object will use 
			//objId
			if (objIds == "null")
			{
				if (objType == "locals")
				{
					AST::StackFrame* pCurStack = rt->GetCurrentStack();
					while (pCurStack != nil)
					{
						if (pCurStack == frameId)
						{
							break;
						}
						pCurStack = pCurStack->Prev();
					}
					if (pCurStack)
					{
						AST::Scope* pCurScope = pCurStack->GetScope();
						SCOPE_FAST_CALL_AddOrGet0(index,pCurScope,objName, true);
						if (index >= 0)
						{
							pCurStack->Set(index, newVal);
						}
					}
				}
				else if (objType == "globals")
				{
					auto* pTopModule = rt->M();
					SCOPE_FAST_CALL_AddOrGet0(index,pTopModule->GetMyScope(),objName, true);
					if (index >= 0)
					{
						rt->Set(pTopModule->GetMyScope(), nullptr, index, newVal);
					}
				}
			}
			else
			{
				auto idList = split(objIds, '.');
				if (idList.size() <= 0)
				{
					return false;
				}
				auto rootId = idList[0];
				unsigned long long ullRootId = 0;
				SCANF(rootId.c_str(), "%llu", &ullRootId);
				Data::Object* pObjRoot = dynamic_cast<Data::Object*>((XObj*)ullRootId);
				if (pObjRoot)
				{
					correctedValue = pObjRoot->UpdateItemValue(rt, nullptr, idList, 1, objName, newVal);
				}
			}
			PackValueAsDict(retDict, objName, correctedValue);
			objRetValue = retDict;
			return true;
		}
		bool DebugService::BuildObjectContent(XlangRuntime* rt,
			XObj* pContextCurrent, AST::StackFrame* frameId, X::Value& valParam,
			X::Value& valObject)
		{
			if (!valParam.IsObject())
			{
				return false;
			}
			//[objid,start Index,count]
			Data::List* pParamObj = dynamic_cast<Data::List*>(valParam.GetObj());
			if (pParamObj == nullptr || pParamObj->Size() == 0)
			{
				return false;
			}
			//first element is varType
			X::Value valObjType;
			pParamObj->Get(0, valObjType);
			std::string varType = valObjType;
			X::Value valObjReq;
			pParamObj->Get(1, valObjReq);
			std::string objIds = valObjReq;
			//format objId-context.childObjId-context.[repeat]
			//objId maybe a pointer or a key(string) for dict, or a number for list
			auto idList = split(objIds, '.');
			if (idList.size() <= 0)
			{
				return false;
			}
			auto rootIdPair = split(idList[0], '-');
			auto rootId = idList[0];
			unsigned long long ullRootId = 0;
			SCANF(rootId.c_str(), "%llu", &ullRootId);
			if (varType == dbg_scope_special_type)
			{
				auto* pCurScope = (AST::Scope*)(ullRootId);
				PackScopeSpecialVars(rt, pContextCurrent, pCurScope, valObject);
				return true;
			}
			Data::Object* pObjRoot = dynamic_cast<Data::Object*>((XObj*)ullRootId);
			XObj* pContextObj = nullptr;
			if (rootIdPair.size() >= 2)
			{
				unsigned long long contextId = 0;
				SCANF(rootIdPair[1].c_str(), "%llu", &contextId);
				pContextObj = (XObj*)contextId;
			}
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
			Data::List* pList = pObjRoot->FlatPack(rt, pContextObj, idList, 1, startIdx, reqCount);
			//pList already hold one refcount when return from FlatPack
			//so don't need X::Value to add refcount
			valObject = X::Value(pList, false);
			return true;
		}
		bool DebugService::BuildStackInfo(
			XlangRuntime* rt, XObj* pContextCurrent,
			CommandInfo* pCommandInfo,
			X::Value& valStackInfo)
		{
			TraceEvent traceEvent = pCommandInfo->m_traceEvent;
			AST::StackFrame* pCurStack = rt->GetCurrentStack();
			Data::List* pList = new Data::List();
			while (pCurStack != nil)
			{
				int line = pCurStack->GetStartLine();
				int column = pCurStack->GetCharPos();

				AST::Scope* pMyScope = pCurStack->GetScope();
				//std::string moduleFileName = rt->M()->GetModuleName();
				if (pMyScope)
				{
					Data::Dict* dict = new Data::Dict();
					dict->Set("id", X::Value(pCurStack));
					std::string name;
					auto* pExp = pMyScope->GetExp();
					std::string moduleFileName = Dbg::GetExpModule(pExp)->GetModuleName();
					if (pExp->m_type == X::AST::ObType::Func)
					{
						AST::Func* pFunc = dynamic_cast<AST::Func*>(pExp);
						name = pFunc->GetNameString();
						if (name.empty())
						{
							char v[1000];
							snprintf(v, sizeof(v), "lambda:(%d,%d)0x%llx",
								pFunc->GetStartLine(), pFunc->GetCharPos(),
								(unsigned long long)pFunc);
							name = v;
						}
					}
					Data::Str* pStrName = new Data::Str(name);
					dict->Set("name", X::Value(pStrName));
					Data::Str* pStrFileName = new Data::Str(moduleFileName);
					dict->Set("file", X::Value(pStrFileName));
					dict->Set("line", X::Value(line));
					dict->Set("column", X::Value(column));
					X::Value valDict(dict);
					pList->Add(rt, valDict);
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

		void DebugService::SetDebug(int iVal)
		{
			Hosting::I().SetDebugMode(iVal > 0);
		}

		X::Value DebugService::GetThreads()
		{
			X::Value valThread;
			Data::List* pList = new Data::List();
			std::unordered_map<long long, XlangRuntime*> mapRt = X::G::I().GetThreadRuntimeIdMap();
			if (mapRt.size() > 0)
			{
				for (auto it = mapRt.begin(); it != mapRt.end(); ++it)
				{
					Data::Dict* dict = new Data::Dict();
					dict->Set("id", X::Value(it->first));
					dict->Set("name", X::Value(it->first));
					//dict->Set("name", X::Value(it->second->GetName()));
					X::Value valDict(dict);
					pList->Add(valDict);
				}
			}
			valThread = X::Value(pList);
			return valThread;
		}

		// Breakpoints should work in both currently running and later created modules
		X::Value DebugService::SetBreakpoints(X::XRuntime* rt, X::XObj* pContext, Value& varPath, Value& varLines)
		{
			if (!varLines.IsObject()
				|| varLines.GetObj()->GetType() != X::ObjType::List)
			{
				return X::Value(false);
			}

			std::string path = varPath.ToString();
			auto* pLineList = dynamic_cast<X::Data::List*>(varLines.GetObj());
			auto lines = pLineList->Map<int>(
				[](X::Value& elm, unsigned long long idx) {
					return elm; }
			);

			G::I().SetBreakPoints(path, lines); // record 
			std::vector<AST::Module*> modules = Hosting::I().QueryModulesByPath(path);
			X::List list;
			if (modules.size() > 0)
			{
				for (auto m : modules)
				{
					m->ClearBreakpoints();
					for (auto l : lines)
					{
						int al = m->SetBreakpoint(l, (int)GetThreadID());
						if (al >= 0)
						{
							list += l;
							list += al; // actual line
						}
						else
						{
							list += l;
							list += -1;// failed state
						}
					}
					G::I().AddBreakpointValid(path); // record this source file's breakpoints has been checked
				}
			}
			else
			{
				for (auto l : lines)
				{
					list += l;
					list += -2;// pending state
				}
			}

			return X::Value(list);
		}
		bool DebugService::Command(X::XRuntime* rt, XObj* pContext,
			ARGS& params, KWARGS& kwParams, X::Value& retValue)
		{
			if (params.size() == 0)
			{
				retValue = X::Value(false);
				return true;
			}
			//we had use first parameter as ModuleKey,
			//now change to threadId
			unsigned long long threadId = params[0].GetLongLong();
			//AST::Module* pModule = X::G::I().QueryModuleByThreadId(threadId);
			XlangRuntime* threadRt = (XlangRuntime*)X::G::I().QueryRuntimeForThreadId(threadId);
			if (threadRt == nullptr)
			{
				retValue = X::Value(false);
				return true;
			}
			
			AST::Module* pModule;
			if (!(pModule = threadRt->M()))
			{
				retValue = X::Value(false);
				return true;
			}
			std::string strCmd;
			auto it = kwParams.find("cmd");
			if (it)
			{
				strCmd = it->val.ToString();
			}
			X::Value valParam;
			it = kwParams.find("param");
			if (it)
			{
				valParam = it->val;
			}
			if (strCmd == "Stack")
			{
				auto stackTracePack = [](XlangRuntime* rt,
					XObj* pContextCurrent,
					CommandInfo* pCommandInfo,
					X::Value& retVal)
				{
					DebugService* pDebugService = (DebugService*)
						pCommandInfo->m_callContext;
					pDebugService->BuildStackInfo(rt, pContextCurrent,
						pCommandInfo, retVal);
				};
				CommandInfo* pCmdInfo = new CommandInfo();
				pCmdInfo->m_callContext = this;
				pCmdInfo->m_process = stackTracePack;
				pCmdInfo->m_threadId = threadId;
				pCmdInfo->m_needRetValue = true;
				pCmdInfo->dbgType = dbg::StackTrace;
				pCmdInfo->IncRef();//we need keep it for return, will removing in below
				threadRt->AddCommand(pCmdInfo, true);
				retValue = pCmdInfo->m_retValueHolder;
				pCmdInfo->DecRef();
			}
			else if (strCmd == "Globals"
				|| strCmd == "Locals"
				|| strCmd == "Object"
				|| strCmd == "SetObjectValue")
			{
				AST::StackFrame* frameId = 0;
				auto it2 = kwParams.find("frameId");
				if (it2)
				{
					frameId = (AST::StackFrame*)it2->val.GetLongLong();
				}
				CommandInfo* pCmdInfo = new CommandInfo();
				pCmdInfo->m_frameId = frameId;
				pCmdInfo->dbgType = dbg::GetRuntime;
				auto globalPack = [](XlangRuntime* rt,
					XObj* pContextCurrent,
					CommandInfo* pCommandInfo,
					X::Value& retVal)
				{
					DebugService* pDebugService = (DebugService*)
						pCommandInfo->m_callContext;
					pDebugService->BuildGlobals(rt, pContextCurrent, retVal);
				};
				auto localPack = [](XlangRuntime* rt,
					XObj* pContextCurrent,
					CommandInfo* pCommandInfo,
					X::Value& retVal)
				{
					DebugService* pDebugService = (DebugService*)
						pCommandInfo->m_callContext;
					pDebugService->BuildLocals(rt, pContextCurrent,
						pCommandInfo->m_frameId, retVal);
				};
				auto objPack = [](XlangRuntime* rt,
					XObj* pContextCurrent,
					CommandInfo* pCommandInfo,
					X::Value& retVal)
				{
					DebugService* pDebugService = (DebugService*)
						pCommandInfo->m_callContext;
					pDebugService->BuildObjectContent(rt, pContextCurrent,
						pCommandInfo->m_frameId,
						pCommandInfo->m_varParam,
						retVal);
				};
				auto objSetValuePack = [](XlangRuntime* rt,
					XObj* pContextCurrent,
					CommandInfo* pCommandInfo,
					X::Value& retVal)
				{
					DebugService* pDebugService = (DebugService*)
						pCommandInfo->m_callContext;
					pDebugService->ObjectSetValue(rt, pContextCurrent,
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
				else if (strCmd == "SetObjectValue")
				{
					pCmdInfo->m_process = objSetValuePack;
				}
				pCmdInfo->m_varParam = valParam;
				pCmdInfo->m_callContext = this;
				pCmdInfo->m_needRetValue = true;
				pCmdInfo->IncRef();// we need pCmdInfo keep for return
				threadRt->AddCommand(pCmdInfo, true);
				retValue = pCmdInfo->m_retValueHolder;
				pCmdInfo->DecRef();
			}
			else if (strCmd == "Step")
			{
				CommandInfo* pCmdInfo = new CommandInfo();
				//we don't need return from pCmdInfo, so dont' call IncRef for pCmdInfo
				//and when this command be processed, will release it
				pCmdInfo->dbgType = dbg::Step;
				threadRt->AddCommand(pCmdInfo, false);
				retValue = X::Value(true);
			}
			else if (strCmd == "Continue")
			{
				CommandInfo* pCmdInfo = new CommandInfo();
				//we don't need return from pCmdInfo, so dont' call IncRef for pCmdInfo
				//and when this command be processed, will release it
				pCmdInfo->dbgType = dbg::Continue;
				threadRt->AddCommand(pCmdInfo, false);
				retValue = X::Value(true);
			}
			else if (strCmd == "StepIn")
			{
				CommandInfo* pCmdInfo = new CommandInfo();
				//we don't need return from pCmdInfo, so dont' call IncRef for pCmdInfo
				//and when this command be processed, will release it
				pCmdInfo->dbgType = dbg::StepIn;
				threadRt->AddCommand(pCmdInfo, false);
				retValue = X::Value(true);
			}
			else if (strCmd == "StepOut")
			{
				CommandInfo* pCmdInfo = new CommandInfo();
				//we don't need return from pCmdInfo, so dont' call IncRef for pCmdInfo
				//and when this command be processed, will release it
				pCmdInfo->dbgType = dbg::StepOut;
				threadRt->AddCommand(pCmdInfo, false);
				retValue = X::Value(true);
			}
			else if (strCmd == "Terminate")
			{
				//AST::CommandInfo* pCmdInfo = new AST::CommandInfo();
				//we don't need return from pCmdInfo, so dont' call IncRef for pCmdInfo
				//and when this command be processed, will release it
				//pCmdInfo->dbgType = dbg::Terminate;
				//pModule->AddCommand(pCmdInfo, false); // todo��stop run every module
				//retValue = X::Value(true);
			}
			return true;
		}
	}
}

