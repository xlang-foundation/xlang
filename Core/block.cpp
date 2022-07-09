#include "block.h"
#include "var.h"
#include "func.h"
#include "builtin.h"
#include "module.h"
#include <iostream>
#include "utility.h"
#include "dbg.h"

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
	Body.push_back(item);
	item->SetParent(this);
	item->ScopeLayout();

}
bool Block::Run(Runtime* rt,void* pContext, Value& v, LValue* lValue)
{
	bool bOk = true;
	m_bRunning = true;
	for (auto i : Body)
	{
		rt->SetCurrentExpr(i);
		Dbg(rt).Check(rt,i, pContext);
		//int line = i->GetStartLine();
		//std::cout << "Run Line:" << line <<std::endl;
		Value v0;
		bOk = i->Run(rt,pContext, v0);
		if (!bOk)
		{//TODO: error process here
			break;
		}
	}
	m_bRunning = false;

	return bOk;
}
bool While::Run(Runtime* rt,void* pContext,Value& v,LValue* lValue)
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
bool For::Run(Runtime* rt,void* pContext,Value& v,LValue* lValue)
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
bool If::Run(Runtime* rt,void* pContext,Value& v,LValue* lValue)
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