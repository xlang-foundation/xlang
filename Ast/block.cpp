#include "block.h"
#include "var.h"
#include "func.h"
#include "builtin.h"
#include "module.h"
#include <iostream>
#include "utility.h"

extern bool U_Print(X::XRuntime* rt, X::XObj* pContext,
	X::ARGS& params,
	X::KWARGS& kwParams,
	X::Value& retValue);

namespace X
{
namespace AST
{
void Block::Add(Expression* item)
{
	int line = item->GetStartLine();
	if (Body.size() > 0)
	{//for if elif else 
		Expression* pLastExp = Body[Body.size() - 1];
		if (pLastExp->EatMe(item))
		{
			item->ScopeLayout();
			return;
		}
	}
	ReCalcHint(item);
	Body.push_back(item);
	item->SetParent(this);
	item->ScopeLayout();

}
bool Block::Run(XRuntime* rt0,XObj* pContext, Value& v, LValue* lValue)
{
	if (Body.size() == 0)
	{
		return false;
	}
	XlangRuntime* rt = (XlangRuntime*)rt0;
	bool bOk = true;
	m_bRunning = true;

	if (!rt->GetTrace())
	{//just for debug easy,write same code here
	// because dbg also run xlang code
	//then easy to set breakpoint for xlang code in debug mode
	//not for dbg xlang code
		auto last = Body[Body.size() - 1];
		for (auto& i : Body)
		{
			Value v0;
			int line = i->GetStartLine();
			bOk = i->Run(rt, pContext, v0);
			if (!bOk)
			{
				std::cout << "Error Occurs in line:" << line << std::endl;
			}
			if (v0.IsValid() && (i == last))
			{
				v = v0;
			}
		}
		m_bRunning = false;
		return bOk;
	}
	//if being traced, go to here
	bool bEnterBlock = false;
	if ((m_type == ObType::Func || m_type == ObType::Module) &&
		rt->M()->GetDbgType() == X::AST::dbg::StepIn)
	{
		bEnterBlock = true;
		rt->GetTrace()(rt, pContext,rt->GetCurrentStack(),
			TraceEvent::Call,
			dynamic_cast<Scope*>(this),nullptr);
	}
	auto last = Body[Body.size() - 1];
	for (auto& i : Body)
	{
		//Update Stack Frame
		int line = i->GetStartLine();
		int pos = i->GetCharPos();
		rt->GetCurrentStack()->SetLine(line);
		rt->GetCurrentStack()->SetCharPos(pos);
		//std::cout << "Run Line(before check):" << line <<std::endl;

		if (rt->GetTrace())
		{
			Scope* pMyScope = nullptr;
			if (m_type == ObType::Func
				|| m_type == ObType::Module)
			{
				pMyScope = dynamic_cast<Scope*>(this);
			}
			else
			{
				pMyScope = GetScope();
			}
			rt->GetTrace()(rt, pContext, rt->GetCurrentStack(),
				TraceEvent::Line, pMyScope, i);
		}
		Value v0;
		bOk = i->Run(rt,pContext, v0);
		//std::cout << "after run line:" << line << std::endl;
		if (!bOk)
		{//TODO: error process here
			std::cout << "Error Occurs in line:" << line << std::endl;
			//break;
		}
		if (v0.IsValid() && (i == last))
		{
			v = v0;
		}
	}
	m_bRunning = false;
	if (rt->GetTrace() && bEnterBlock
		&& (m_type == ObType::Func
		|| m_type == ObType::Module))
	{
		rt->GetTrace()(rt, pContext, rt->GetCurrentStack(),
			TraceEvent::Return,
			dynamic_cast<Scope*>(this),nullptr);
	}
	return bOk;
}

bool Block::RunFromLine(XRuntime* rt, XObj* pContext,
	long long lineNo, Value& v, LValue* lValue)
{
	auto print_var = [](XRuntime* rt, XObj* pContext,Expression* exp)
	{
		if (exp->m_type == ObType::Var)
		{
			//just print out
			Value v0;
			bool bOK = exp->Run((XlangRuntime*)rt, pContext, v0);
			if (bOK)
			{
				X::ARGS args_p;
				args_p.push_back(v0);
				X::KWARGS kwArgs_p;
				X::Value retValue_p;
				U_Print(rt, pContext, args_p, kwArgs_p, retValue_p);
			}
			return bOK;
		}
		else
		{
			return false;
		}
	};
	bool bOK = false;
	auto lineCnt = Body.size();
	for (long long i = lineNo; i < lineCnt; i++)
	{
		auto line = Body[i];
		if (line->m_type == ObType::Var)
		{
			print_var(rt, pContext, line);
			continue;
		}
		else if (line->m_type == ObType::List)
		{
			List* pList = dynamic_cast<List*>(line);
			auto& list = pList->GetList();
			for (auto& p : list)
			{
				print_var(rt, pContext, p);
			}
			continue;
		}
		Value v0;
		bOK = line->Run((XlangRuntime*)rt, pContext, v0);
		if (!bOK)
		{
			break;
		}
		if (i == (lineCnt - 1))
		{
			v = v0;
		}
	}
	return bOK;
}
bool Block::RunLast(XRuntime* rt0, XObj* pContext, Value& v, LValue* lValue)
{
	if (Body.size() == 0)
	{
		return false;
	}
	auto last = Body[Body.size() - 1];
	Value v0;

	bool bOk = last->Run((XlangRuntime*)rt0, pContext, v0);
	if (bOk)
	{
		v = v0;
	}
	return bOk;
}
bool While::Run(XlangRuntime* rt,XObj* pContext,Value& v,LValue* lValue)
{
	if (R == nil)
	{
		return false;
	}
	Value v0;
	while (true)
	{
		Value v0;
		bool bOK = R->Run(rt,pContext,v0);
		if (bOK && v0.IsTrue())
		{
			Block::Run(rt,pContext,v);
		}
		else
		{
			break;
		}
	}
	return true;
}
bool For::Run(XlangRuntime* rt,XObj* pContext,Value& v,LValue* lValue)
{
	Value v0;
	while (true)
	{
		bool bContinue = false;
		bool bC0 = R->Run(rt,pContext,v0);
		if (bC0)
		{
			if (v0.IsObject())
			{
				ARGS params;
				KWARGS kwParams;
				X::Value retBoolValue;
				if (v0.GetObj()->Call(rt, pContext, 
					params, kwParams, retBoolValue) 
					&& retBoolValue.IsTrue())
				{
					bContinue = true;
				}
			}
			else//range case
			{
				bContinue = true;
			}
		}
		if (bContinue)
		{
			Block::Run(rt, pContext, v);
		}
		else
		{
			break;
		}
	}
	return true;
}

bool If::EatMe(Expression* other)
{
	If* elseIf = dynamic_cast<If*>(other);
	if (elseIf)
	{
		If* p = this;
		If* n = m_next;
		while (n != nil)
		{
			p = n;
			n = n->m_next;
		}
		p->m_next = elseIf;
		elseIf->SetParent(p);
		return true;
	}
	else
	{
		return false;
	}
}
bool If::Run(XlangRuntime* rt,XObj* pContext,Value& v,LValue* lValue)
{
	bool bRet = true;
	bool bCanRun = false;
	if (R)
	{
		Value v0;
		bool bOK = R->Run(rt,pContext,v0);
		if (bOK && v0 == Value(true))
		{
			bCanRun = true;
		}
	}
	else
	{//for Else in if 
		bCanRun = true;
	}
	if (bCanRun)
	{
		bRet = Block::Run(rt,pContext,v);
	}
	else if(m_next)
	{
		bRet = m_next->Run(rt,pContext,v);
	}
	return bRet;
}
}
}