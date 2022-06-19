#pragma once
#include "exp.h"
#include "scope.h"
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
	void ScopeLayout(std::vector<AST::Expression*>& candidates);
	virtual void ScopeLayout() override;
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
