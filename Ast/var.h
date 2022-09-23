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
	bool m_needRelease = false;//Name.s is created by this Var,then = true
	int Index = -1;//index for this Var,set by compiling

	bool GetPropValue(Runtime* rt, XObj* pContext,XObj* pObj,Value& val);
public:
	Var()
	{
		m_type = ObType::Var;
	}
	Var(String& n)
	{
		Name = n;
		m_type = ObType::Var;
	}
	~Var()
	{
		if (m_needRelease)
		{
			delete Name.s;
		}
	}
	void EncodeExtern(Runtime* rt, XObj* pContext, X::XLangStream& stream);
	void DecodeExtern(Runtime* rt, XObj* pContext, X::XLangStream& stream);
	virtual bool ToBytes(Runtime* rt, XObj* pContext, X::XLangStream& stream);
	virtual bool FromBytes(X::XLangStream& stream);
	void ScopeLayout(std::vector<Scope*>& candidates);
	virtual void ScopeLayout() override;
	String& GetName() { return Name; }
	std::string GetNameString() { return std::string(Name.s, Name.size); }
	inline virtual void Set(Runtime* rt, XObj* pContext, Value& v) override
	{
		if (Index == -1)
		{
			ScopeLayout();
			assert(Index != -1 && m_scope != nullptr);
		}
		m_scope->Set(rt, pContext, Index, v);
	}
	virtual bool CalcCallables(Runtime* rt, XObj* pContext,
		std::vector<Scope*>& callables) override;
	inline virtual bool Run(Runtime* rt, XObj* pContext, Value& v, LValue* lValue = nullptr) override
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
		if(m_parent && m_parent->m_type == ObType::Dot 
			&& (!m_parent->IsLeftValue()) && v.IsObject()
			&& v.GetObj()->GetType() == ObjType::Prop)
		{//for right value, if it is prop, need to calc out
			GetPropValue(rt, pContext, v.GetObj(), v);
			if (lValue)
			{
				*lValue =v;
			}
		}
		return true;
	}
};
}
}
