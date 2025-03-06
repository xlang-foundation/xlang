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

#pragma once
#include "exp.h"
#include "runtime.h"
#include "manager.h"
#include "module.h"
#include "scope.h"
#include "function.h"
#include <vector>
#include <iostream>
#include "pyproxyobject.h"
#include "glob.h"

namespace X
{
class Dbg
{
	XlangRuntime* m_rt = nullptr;
public:
	FORCE_INLINE Dbg(XlangRuntime* rt)
	{
		m_rt = rt;
	}
	static bool xTraceFunc(
		XlangRuntime* rt,
		XObj* pContext,
		AST::StackFrame* frame,
		TraceEvent traceEvent,
		AST::Scope* pThisBlock,
		AST::Expression* pCurrentObj);

	static int PythonTraceFunc(
		PyEngObjectPtr self,
		PyEngObjectPtr frame,
		int event,
		PyEngObjectPtr args);
	void Loop(X::Value& lineRet,AST::Expression* exp, XObj* pContext)
	{
		std::cout << lineRet.ToString() << std::endl;
		int line = exp->GetStartLine();
		std::cout << "(" << line << ",(c)ontinue,(s)tep)>>";
		while (true)
		{
			std::string input;
			std::getline(std::cin, input);
			if (input == "c" || input == "C")
			{
				m_rt->SetDbgType(dbg::Continue,
					dbg::Continue);
				break;
			}
			else if (input == "s" || input == "S")
			{
				m_rt->SetDbgType(dbg::Step,
					dbg::Step);
				break;
			}
			else if(input == "l" || input == "L")
			{
				AST::Scope* pMyScope = exp->GetScope();
				auto l_names = pMyScope->GetVarNames();
				for (auto& l_name : l_names)
				{
					X::Value vDbg;
					SCOPE_FAST_CALL_AddOrGet0(idx,pMyScope,l_name, true);
					m_rt->Get(pMyScope, pContext, idx, vDbg);
					std::cout << l_name << ":" << vDbg.ToString() << std::endl;
				}
			}
			else
			{
				AST::Scope* pMyScope = exp->GetScope();
				auto pos = input.find("show");
				if (pos != std::string::npos)
				{
					std::string strParas = input.substr(pos + 4);
					std::vector<std::string> params =
						split(strParas, ',');
					if (params.size() == 0)
					{
						params.push_back(strParas);
					}
					for (auto& param : params)
					{
						X::Value vDbg;
						if (pMyScope)
						{
							SCOPE_FAST_CALL_AddOrGet0(idx,pMyScope,param, true);
							m_rt->Get(pMyScope, pContext, idx, vDbg);
						}
						std::cout <<vDbg.ToString() << std::endl;
					}
				}

			}
			std::cout << ">>";
		}
	}
	void WaitForCommnd(TraceEvent evt,XlangRuntime* rt, 
		AST::Scope* pThisBlock,
		AST::Expression* exp, XObj* pContext)
	{
		auto* pModule = rt->M();
		CommandInfo* pCmdInfo;
		bool mLoop = true;
		while (mLoop)
		{
			pCmdInfo = rt->PopCommand();
			if (pCmdInfo == nullptr)
			{
				//todo:
				continue;
				//break;
			}
			//get runtime for this thread with threadId in pCmdInfo
			XlangRuntime* rtForDebugThread =dynamic_cast<XlangRuntime*>(G::I().QueryRuntimeForThreadId(pCmdInfo->m_threadId));
			if (rtForDebugThread == nullptr)
			{
				//happend when debugger is launching 
				rtForDebugThread = rt;
			}
			switch (pCmdInfo->dbgType)
			{
			case dbg::GetRuntime:
			case dbg::StackTrace:
				//just get back the current exp, then
				//will calcluate out stack frames
				//by call AddCommand
				if (pCmdInfo->m_process)
				{
					X::Value retVal;
					pCmdInfo->m_process(rtForDebugThread, pContext, pCmdInfo, retVal);
					if (pCmdInfo->m_needRetValue)
					{
						pCmdInfo->m_retValueHolder = retVal.ToString(true);
					}
				}
				break;
			case dbg::Continue:
				m_rt->SetDbgType(dbg::Continue,	dbg::Continue);
				mLoop = false;
				break;
			case dbg::Step:
			{
				std::vector<AST::Scope*> callables;
				exp->CalcCallables(rtForDebugThread, pContext, callables);
				if (!(exp->m_type == AST::ObType::Func || exp->m_type == AST::ObType::Class) && callables.size() > 0 && callables[0]->GetExp() && callables[0]->GetExp()->m_type == AST::ObType::Func)// can trace into
				{
					m_rt->SetDbgType(dbg::StepOut, dbg::Step); // set DbgType to StepOut to skip trace in this exp
					m_rt->m_pFirstStepOutExp = callables[0]->GetExp();
				}
				else
					m_rt->SetDbgType(dbg::Step, dbg::Step);// can not trace into
				mLoop = false;
				break;
			}
			case dbg::StepIn:
			{
				std::vector<AST::Scope*> callables;
				exp->CalcCallables(rtForDebugThread, pContext, callables);
				if (callables.size() > 0 && callables[0]->GetExp()->m_type == AST::ObType::Func)
					m_rt->SetDbgType(dbg::StepIn, dbg::StepIn);// can trace into
				else
					m_rt->SetDbgType(dbg::Step, dbg::StepIn);// can not trace into
				mLoop = false;
				break;
			}
			case dbg::StepOut:
				m_rt->SetDbgType(dbg::StepOut,	dbg::StepOut);
				mLoop = false;
				break;
			case dbg::Terminate:
				//m_rt->SetDbgType(dbg::Terminate, dbg::Terminate);
				//mLoop = false;
				break;
			default:
				break;
			}
			if (pCmdInfo->m_wait)
			{
				pCmdInfo->m_wait->Release();
			}
			pCmdInfo->DecRef();
		}
	}
	FORCE_INLINE bool ExitBlockRun(XlangRuntime* rt,XObj* pContext,
		AST::Scope* pThisBlock)
	{
		if (G::I().GetTrace())
		{
			m_rt->M()->RemoveDbgScope(pThisBlock);
		}
		return true;
	}

	FORCE_INLINE static AST::Module* GetExpModule(AST::Expression* exp)
	{
		if (exp->m_type == AST::ObType::Module)
			return dynamic_cast<AST::Module*>(exp);

		auto pa = exp->GetParent();
		while (pa)
		{
			if (pa->m_type == AST::ObType::Module)
			{
				return dynamic_cast<AST::Module*>(pa);
				break;
			}
			else
			{
				pa = pa->GetParent();
			}
		}
		return nullptr;
	}

	FORCE_INLINE bool Check(TraceEvent evt,XlangRuntime* rt,
		AST::Scope* pThisBlock,
		AST::Expression* exp, XObj* pContext)
	{
		if (!G::I().GetTrace())
			return true;

		AST::Module* expModule = GetExpModule(exp);
		if (!expModule || !G::I().GetTrace())
		{
			return false;
		}
		//check breakpoints
		int line = exp->GetStartLine();
		if (G::I().GetTrace() && expModule->HitBreakpoint(rt,line)) // if attached, check if a breakpoint hitted
		{
			WaitForCommnd(evt, rt, pThisBlock, exp, pContext);
			if (m_rt->GetDbgType() == X::dbg::Terminate)
				return false;
			else
			    return true;
		}
		auto st = G::I().GetTrace() ? m_rt->GetDbgType() : X::dbg::Continue; // if detached, set DbgType to Continue
		switch (st)
		{
		case X::dbg::Continue:
			break;
		case X::dbg::Step:
		{
			if (m_rt->M()->InDbgScope(pThisBlock))
			{
				// stopOnEntry
				if (m_rt->GetLastRequestDgbType() == X::dbg::None)
					m_rt->M()->StopOn("StopOnEntry");
				else
					m_rt->M()->StopOn("StopOnStep");

				WaitForCommnd(evt, rt, pThisBlock, exp, pContext);
				if (m_rt->GetDbgType() == X::dbg::Terminate)
					return false;
				else
					return true;
			}
		}
		break;
		case X::dbg::Terminate:
		{
			return false;
		}
		break;
		default:
			break;
		}
		return true;
	}
};
}