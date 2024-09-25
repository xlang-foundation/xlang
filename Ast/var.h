/*
Copyright (C) 2024 The XLang Foundation
Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

#pragma once
#include "exp.h"
#include "scope.h"
#include <assert.h> 
#include "exp_exec.h"
namespace X
{
namespace AST
{
class Var:
	public Expression
{
	String Name;
	bool m_needRelease = false;//Name.s is created by this Var,then = true
	int Index = -1;//index for this Var,set by compiling

	bool GetPropValue(XlangRuntime* rt, XObj* pContext,XObj* pObj,Value& val);
public:
	//Expression Excuting
	bool PreExec(X::Exp::ExpresionStack& stack,XlangRuntime* rt)
	{
		return false;
	}
	bool PostExec(X::Exp::ExpresionStack& stack,XlangRuntime* rt)
	{
		return false;
	}
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
	void MergeWithPreviousToken(Var* pPreviousVar)
	{
		//pPreviousVar must be the previous tokoen
		//so can keep the continues memory
		Name.size =int( (Name.s + Name.size) - pPreviousVar->Name.s);
		Name.s = pPreviousVar->Name.s;
		m_lineStart = pPreviousVar->m_lineStart;
		m_charStart = pPreviousVar->m_charStart;
		m_charPos = pPreviousVar->m_charPos;
		m_tokenIndex = pPreviousVar->m_tokenIndex;
	}
	~Var()
	{
		if (m_needRelease)
		{
			delete Name.s;
		}
	}
	void EncodeExtern(XlangRuntime* rt, XObj* pContext, X::XLangStream& stream);
	void DecodeExtern(XlangRuntime* rt, XObj* pContext, X::XLangStream& stream);
	virtual void SetScope(Scope* p) override
	{
		Expression::SetScope(p);
		Index = -1;
	}
	virtual bool ToBytes(XlangRuntime* rt, XObj* pContext, X::XLangStream& stream);
	virtual bool FromBytes(X::XLangStream& stream);
	void ScopeLayout(std::vector<Scope*>& candidates);
	virtual void ScopeLayout() override;
	String& GetName() { return Name; }
	std::string GetNameString() { return std::string(Name.s, Name.size); }
	FORCE_INLINE virtual bool Set(XlangRuntime* rt, XObj* pContext, Value& v) override final
	{
		if (Index == -1)
		{
			ScopeLayout();
			//still not find, return false
			if (Index == -1)
			{
				return false;
			}
		}
		return m_scope->Set(rt,pContext, Index, v);
	}
	virtual bool SetArry(XlangRuntime* rt, XObj* pContext, std::vector<Value>& ary) override
	{
		//only take the first one 
		if (ary.size() == 0)
		{
			return false;
		}
		if (Index == -1)
		{
			ScopeLayout();
			assert(Index != -1 && m_scope != nullptr);
		}
		return rt->Set(m_scope, pContext, Index, ary[0]);
	}
	virtual bool CalcCallables(XlangRuntime* rt, XObj* pContext,
		std::vector<Scope*>& callables) override;
	FORCE_INLINE virtual bool Exec(XlangRuntime* rt,ExecAction& action, XObj* pContext, Value& v, LValue* lValue = nullptr) override final
	{
		if (Index == -1 || m_scope == nullptr || rt == nullptr)
		{//case 1:RunExpression in XHost will call with rt is null,
		//in this case, always search Index from scope
		//because this scope may changed from caller side
			ScopeLayout();
			if (Index == -1 || m_scope == nullptr)
			{
				return false;
			}
		}
		m_scope->Get(rt, pContext, Index, v, lValue);

		if(m_parent && m_parent->m_type == ObType::Dot
			&& (!m_parent->IsLeftValue()) && v.IsObject()
			&& (v.GetObj()->GetType() == ObjType::Prop ||
				v.GetObj()->GetType() == ObjType::StructField))
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
