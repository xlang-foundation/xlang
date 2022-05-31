#include "exp.h"


namespace XPython {namespace AST {

Scope* Expression::FindScope()
{
	Scope* pMyScope = nil;
	Expression* pa = m_parent;
	while (pa != nil && pMyScope == nil)
	{
		pMyScope = dynamic_cast<Scope*>(pa);
		pa = pa->GetParent();
	}
	return pMyScope;
}

void Var::Set(Value& v)
{
	auto pScope = FindScope();
	if (pScope)
	{
		std::string key(Name.s, Name.size);
		pScope->Set(key, v);
	}
}

bool Var::Run(Value& v)
{
	bool bOK = false;
	std::string key(Name.s, Name.size);
	auto pScope = FindScope();
	if (pScope)
	{
		bOK = pScope->Get(key, v);
	}
	return bOK;
}

bool Func::Call(List* params, Value& retValue)
{
	Value v0;
	Block::Run(v0);
	return true;
}

bool PairOp::Run(Value& v)
{
	if (A == Alias::Parenthesis_L)
	{//Call Func
		if (L && L->m_type == ObType::Var)
		{
			Block* pMyBlock = nil;
			Expression* pa = m_parent;
			while (pa != nil && pMyBlock == nil)
			{
				pMyBlock = dynamic_cast<AST::Block*>(pa);
				pa = pa->GetParent();
			}
			if (pMyBlock)
			{
				Func* pFunc = pMyBlock->FindFuncByName((Var*)L);
				if (pFunc)
				{
					Value retValue;
					if (pFunc->Call((List*)R,retValue))
					{//v is return value if changed
							
					}
				}				
			}
		}
	}
	return true;
}

Func* Block::FindFuncByName(Var* name)
{
	Func* func = nil;
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
		String& target_name = name->GetName();
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

}
}