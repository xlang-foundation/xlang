#pragma once
#include "exp.h"
#include "scope.h"
#include <assert.h> 
namespace X
{
namespace AST
{
class Var :
	virtual public Expression
{
	String Name;
	int Index = -1;//index for this Var,set by compiling
public:
	Var(String& n)
	{
		Name = n;
		m_type = ObType::Var;
	}
	void ScopeLayout(std::vector<Scope*>& candidates);
	virtual void ScopeLayout() override;
	String& GetName() { return Name; }
	std::string GetNameString() { return std::string(Name.s, Name.size); }
	inline virtual void Set(Runtime* rt, void* pContext, Value& v) override
	{
		if (Index == -1)
		{
			ScopeLayout();
			assert(Index != -1 && m_scope != nullptr);
		}
		m_scope->Set(rt, pContext, Index, v);
	}
	virtual bool CalcCallables(Runtime* rt, void* pContext,
		std::vector<Scope*>& callables) override;
	inline virtual bool Run(Runtime* rt, void* pContext, Value& v, LValue* lValue = nullptr) override
	{
		if (Index == -1 || m_scope == nullptr)
		{
			ScopeLayout();
			if (Index == -1 || m_scope == nullptr)
			{//treat as string
				v = Value(Name.s, Name.size);
				return true;
			}
		}
		m_scope->Get(rt, pContext, Index, v, lValue);
		return true;
	}
};
}
}
