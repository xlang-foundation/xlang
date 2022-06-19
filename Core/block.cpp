#include "block.h"
#include "var.h"
#include "func.h"
#include "builtin.h"
#include "module.h"
#include <iostream>

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
Func* Block::FindFuncByName(Var* name)
{
	Func* func = nil;
	String& target_name = name->GetName();
	//internl first
	std::string strName(target_name.s, target_name.size);
	ExternFunc* extFunc = Builtin::I().Find(strName);
	if (extFunc)
	{
		return extFunc;
	}
	for (auto i : Body)
	{
		if (i->m_type != ObType::Func)
		{
			continue;
		}
		auto iFunc = (Func*)i;
		Var* ivarName = dynamic_cast<Var*>(iFunc->GetName());
		if (ivarName == nil)
		{
			continue;
		}
		String& i_name = ivarName->GetName();
		bool bMatched = false;
		if (i_name.size == target_name.size && i_name.size>0)
		{
			bMatched = true;
			for (int j = 0; j < i_name.size; j++)
			{
				if (i_name.s[j] != target_name.s[j])
				{
					bMatched = false;
					break;
				}
			}
		}
		if (bMatched)
		{
			func = iFunc;
			break;
		}
	}
	return func;
}
bool Block::Run(Runtime* rt,void* pContext, Value& v, LValue* lValue)
{
	bool bOk = true;
	for (auto i : Body)
	{
		Value v0;
		bOk = i->Run(rt,pContext, v0);
		if (!bOk)
		{//TODO: error process here
			break;
		}
		if (rt->M()->GetDbg() == dbg::Step)
		{
			std::cout << v0.ToString() << std::endl;
			int line = i->GetStartLine();
			std::cout << "(" << line << ",(c)ontinue,(s)tep)>>";
			X::AST::Value v0;
			std::string yes;
			std::cin >> yes;
			if (yes == "c" || yes == "C")
			{
				rt->M()->SetDbg(AST::dbg::Continue);
			}
			else if (yes == "s" || yes == "S")
			{
				rt->M()->SetDbg(AST::dbg::Step);
			}
		}
	}
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