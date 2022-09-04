#pragma once
#include "scope.h"
#include "singleton.h"
#include "stackframe.h"

namespace X
{
	namespace AST
	{
		class MetaScope :
			public Scope,
			public Singleton<MetaScope>
		{
			StackFrame* m_stack = nullptr;
		public:
			MetaScope()
			{
				m_stack = new StackFrame();
			}
			~MetaScope()
			{
				delete m_stack;
			}
			virtual Scope* GetParentScope()
			{
				return nullptr;
			}
			virtual int AddOrGet(std::string& name, bool bGetOnly) override
			{
				int retIdx = Scope::AddOrGet(name, bGetOnly);
				if (!bGetOnly)
				{
					m_stack->SetVarCount(GetVarNum());
				}
				return retIdx;
			}
			inline virtual bool Get(Runtime* rt, XObj* pContext,
				int idx, X::Value& v, LValue* lValue = nullptr) override
			{
				m_stack->Get(idx, v, lValue);
				return true;
			}
			inline virtual bool Set(Runtime* rt, XObj* pContext,
				int idx, X::Value& v) override
			{
				m_stack->Set(idx, v);
				return true;
			}
		};
	}
}