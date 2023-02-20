#pragma once

#include "module.h"
#include "block.h"

namespace X
{
	namespace AST
	{
		class ModuleProxy :
			virtual public Module
		{
		protected:
			Scope* m_realScope = nullptr;
			virtual int AddOrGet(std::string& name, bool bGetOnly)
			{
				if (m_realScope)
				{
					return m_realScope->AddOrGet(name, bGetOnly);
				}
				return -1;
			}
			virtual bool Set(XlangRuntime* rt, XObj* pContext, int idx, Value& v) override
			{
				if (m_realScope)
				{
					return m_realScope->Set(rt, pContext, idx, v);
				}
				return false;
			}
			virtual bool Get(XlangRuntime* rt, XObj* pContext, int idx, Value& v,
				LValue* lValue = nullptr) override
			{
				if (m_realScope)
				{
					return m_realScope->Get(rt, pContext, idx, v, lValue);
				}
				return false;
			}
		public:
			virtual void ScopeLayout() override
			{
				//don't need to add AddBuiltins etc.
				Block::ScopeLayout();
			}
			void SetScope(Scope* scope)
			{
				m_realScope = scope;
			}
		};
	}
}