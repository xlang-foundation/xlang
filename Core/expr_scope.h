#pragma once
/*
	scope for emulation of expression 
	for example:
	 x>=1 and y<10
*/

#include "scope.h"
#include "xlang.h"

namespace X
{
	namespace Data
	{
		class ExpressionScope :
			virtual public AST::Scope
		{
			XCustomScope* m_customScope = nullptr;
			virtual Scope* GetParentScope() override { return nullptr; }
			// Inherited via Scope
			virtual int AddOrGet(std::string& name, bool bGetOnly) override
			{
				return m_customScope->Query(name);
			}
			virtual bool Set(XlangRuntime* rt, XObj* pContext, int idx, Value& v) override
			{
				m_customScope->Set(idx, v);
				return true;
			}
			virtual bool Get(XlangRuntime* rt, XObj* pContext, int idx, Value& v,
				LValue* lValue = nullptr) override
			{
				return m_customScope->Get(idx, v);
			}
		public:
			ExpressionScope(XCustomScope* p)
			{
				m_customScope = p;
			}
		};
	}
}