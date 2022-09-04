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
	Runtime* m_rt = nullptr;
public:
	inline Dbg(Runtime* rt)
	{
		m_rt = rt;
	}
	static bool xTraceFunc(
		Runtime* rt,
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
					pMyScope->Get(m_rt, pContext, l_name, vDbg);
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
							pMyScope->Get(m_rt, pContext, param, vDbg);
						}
						std::cout <<vDbg.ToString() << std::endl;
					}
				}

			}
			std::cout << ">>";
		}
	}
	void WaitForCommnd(TraceEvent evt,Runtime* rt, 
		AST::Scope* pThisBlock,
		AST::Expression* exp, XObj* pContext)
	{
		auto* pModule = rt->M();
		AST::CommandInfo cmdInfo;
		bool mLoop = true;
		while (mLoop)
		{
			pModule->PopCommand(cmdInfo);
			switch (cmdInfo.dbgType)
			{
			case AST::dbg::GetRuntime:
			case AST::dbg::StackTrace:
				//just get back the current exp, then
				//will calcluate out stack frames
				//by call AddCommand
				if (cmdInfo.m_process)
				{
					X::Value retVal;
					cmdInfo.m_process(rt, pContext, &cmdInfo, retVal);
					if (cmdInfo.m_retValueHolder)
					{
						*cmdInfo.m_retValueHolder = retVal;
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
				m_rt->M()->SetDbgType(AST::dbg::Step,
					AST::dbg::StepIn);
				mLoop = false;
				break;
			case AST::dbg::StepOut:
				m_rt->M()->RemoveDbgScope(pThisBlock);
				m_rt->M()->SetDbgType(AST::dbg::Step,
					AST::dbg::StepOut);
				mLoop = false;
				break;
			default:
				break;
			}
			if (cmdInfo.m_wait)
			{
				cmdInfo.m_wait->Release();
			}
		}
	}
	inline bool ExitBlockRun(Runtime* rt,XObj* pContext,
		AST::Scope* pThisBlock)
	{
		if (m_rt->M()->IsInDebug())
		{
			m_rt->M()->RemoveDbgScope(pThisBlock);
		}
		return true;
	}
	inline bool Check(TraceEvent evt,Runtime* rt,
		AST::Scope* pThisBlock,
		AST::Expression* exp, XObj* pContext)
	{
		if (!m_rt->M()->IsInDebug())
		{
			return false;
		}
		//check breakpoints
		int line = exp->GetStartLine();
		if (m_rt->M()->HitBreakpoint(line))
		{
			WaitForCommnd(evt,
				rt, pThisBlock, exp, pContext);
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
				WaitForCommnd(evt,
					rt, pThisBlock, exp, pContext);
			}
		}
		break;
		default:
			break;
		}
		return true;
	}
};
}