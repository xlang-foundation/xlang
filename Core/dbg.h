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
				m_rt->M()->SetDbgType(AST::dbg::Continue,
					AST::dbg::Continue);
				break;
			}
			else if (input == "s" || input == "S")
			{
				m_rt->M()->SetDbgType(AST::dbg::Step,
					AST::dbg::Step);
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
		AST::CommandInfo* pCmdInfo;
		bool mLoop = true;
		while (mLoop)
		{
			pCmdInfo = pModule->PopCommand();
			if (pCmdInfo == nullptr)
			{
				//todo:
				continue;
				//break;
			}
			switch (pCmdInfo->dbgType)
			{
			case AST::dbg::GetRuntime:
			case AST::dbg::StackTrace:
				//just get back the current exp, then
				//will calcluate out stack frames
				//by call AddCommand
				if (pCmdInfo->m_process)
				{
					X::Value retVal;
					pCmdInfo->m_process(rt, pContext, pCmdInfo, retVal);
					if (pCmdInfo->m_needRetValue)
					{
						pCmdInfo->m_retValueHolder = retVal.ToString(true);
					}
				}
				break;
			case AST::dbg::Continue:
				m_rt->M()->SetDbgType(AST::dbg::Continue,
					AST::dbg::Continue);
				mLoop = false;
				break;
			case AST::dbg::Step:
				m_rt->M()->SetDbgType(AST::dbg::Step,
					AST::dbg::Step);
				mLoop = false;
				break;
			case AST::dbg::StepIn:
				{
					std::vector<AST::Scope*> callables;
					if (exp->CalcCallables(rt,pContext,callables))
					{
						for (auto& ca : callables)
						{
							m_rt->M()->AddDbgScope(ca);
						}
					}
				}
				//TODO: check here 
				//m_rt->M()->SetDbgType(AST::dbg::Step,AST::dbg::StepIn);
				m_rt->M()->SetDbgType(AST::dbg::StepIn, AST::dbg::StepIn);
				mLoop = false;
				break;
			case AST::dbg::StepOut:
				m_rt->M()->RemoveDbgScope(pThisBlock);
				m_rt->M()->SetDbgType(AST::dbg::Step,
					AST::dbg::StepOut);
				mLoop = false;
				break;
			case AST::dbg::Terminate:
				m_rt->M()->SetDbgType(AST::dbg::Terminate,
					AST::dbg::Terminate);
				mLoop = false;
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
		if (m_rt->M()->IsInDebug())
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
		
		AST::Module* expModule = GetExpModule(exp);
		if (!expModule && !expModule->IsInDebug())
		{
			return false;
		}
		//check breakpoints
		int line = exp->GetStartLine();
		if (expModule->HitBreakpoint(line))
		{
			WaitForCommnd(evt, rt, pThisBlock, exp, pContext);
			if (m_rt->M()->GetDbgType() == X::AST::dbg::Terminate)
				return false;
			else
			        return true;
		}
		auto st = m_rt->M()->GetDbgType();
		switch (st)
		{
		case X::AST::dbg::Continue:
			break;
		case X::AST::dbg::Step:
		{
			if (m_rt->M()->InDbgScope(pThisBlock))
			{
				// stopOnEntry
				if (m_rt->M()->GetLastRequestDgbType() == X::AST::dbg::None)
					m_rt->M()->StopOn("StopOnEntry");
				else
					m_rt->M()->StopOn("StopOnStep");

				WaitForCommnd(evt, rt, pThisBlock, exp, pContext);
				if (m_rt->M()->GetDbgType() == X::AST::dbg::Terminate)
					return false;
				else
					return true;
			}
		}
		break;
		case X::AST::dbg::Terminate:
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