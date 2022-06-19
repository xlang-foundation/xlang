#pragma once
#include "exp.h"
namespace X
{
namespace AST
{
class Var :
	public Expression
{
	String Name;
	int Index = -1;//index for this Var,set by compiling
public:
	Var(String& n)
	{
		Name = n;
		m_type = ObType::Var;
	}
	void ScopeLayout(std::vector<AST::Expression*>& candidates)
	{
		bool matched = false;
		if (m_scope && Index != -1)
		{//check if matche with one candidate
			for (auto it : candidates)
			{
				Scope* s = dynamic_cast<Scope*>(it);
				if (s == m_scope)
				{
					matched = true;
					break;
				}
			}
		}
		if (matched)
		{
			return;
		}
		std::string strName(Name.s, Name.size);
		for (auto it : candidates)
		{
			Scope* s = dynamic_cast<Scope*>(it);
			if (s)
			{
				int idx = s->AddOrGet(strName,
					!m_isLeftValue);
				if (idx != -1)
				{//use the scope to find this name as its scope
					m_scope = s;
					Index = idx;
					break;
				}
			}
		}
	}

	virtual void ScopeLayout() override
	{
		Scope* pMyScope = GetScope();
		int idx = -1;
		while (pMyScope != nullptr && idx == -1)
		{
			std::string strName(Name.s, Name.size);
			idx = pMyScope->AddOrGet(strName,
				!m_isLeftValue);
			if (m_isLeftValue)
			{//Left value will add into local scope
			//don't need to go up
				break;
			}
			if (idx != -1)
			{//use the scope to find this name as its scope
				m_scope = pMyScope;
				break;
			}
			pMyScope = pMyScope->GetParentScope();
		}
		Index = idx;
	}
	String& GetName() { return Name; }
	inline virtual void Set(Runtime* rt, void* pContext, Value& v) override
	{
		m_scope->Set(rt, pContext, Index, v);
	}
	inline virtual bool Run(Runtime* rt, void* pContext, Value& v, LValue* lValue = nullptr) override
	{
		if (Index == -1 || m_scope == nullptr)
		{
			ScopeLayout();
			if (Index == -1 || m_scope == nullptr)
			{
				return false;
			}
		}
		m_scope->Get(rt, pContext, Index, v, lValue);
		return true;
	}
};
}
}
