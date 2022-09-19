#include "block.h"
#include "var.h"
#include "func.h"
#include "builtin.h"
#include "module.h"
#include <iostream>
#include "utility.h"

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
	Runtime* rt = (Runtime*)rt0;
	bool bOk = true;
	m_bRunning = true;
	if (rt->GetTrace() 
		&& (m_type == ObType::Func
		|| m_type == ObType::Module))
	{
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
		//std::cout << "Run Line:" << line <<std::endl;

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
		if (!bOk)
		{//TODO: error process here
			break;
		}
		if (v0.IsValid() && (i == last))
		{
			v = v0;
		}
	}
	m_bRunning = false;
	if (rt->GetTrace() 
		&& (m_type == ObType::Func
		|| m_type == ObType::Module))
	{
		rt->GetTrace()(rt, pContext, rt->GetCurrentStack(),
			TraceEvent::Return,
			dynamic_cast<Scope*>(this),nullptr);
	}
	return bOk;
}
bool While::Run(Runtime* rt,XObj* pContext,Value& v,LValue* lValue)
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
		if (bOK && v0 == Value(true))
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
bool For::Run(Runtime* rt,XObj* pContext,Value& v,LValue* lValue)
{
	Value v0;
	while (true)
	{
		bool bC0 = R->Run(rt,pContext,v0);
		if (!bC0)
		{
			break;
		}
		Block::Run(rt,pContext,v);
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
bool If::Run(Runtime* rt,XObj* pContext,Value& v,LValue* lValue)
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