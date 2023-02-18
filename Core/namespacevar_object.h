#pragma once
#include "object.h"
#include <unordered_map>
#include <vector>
#include "scope.h"
#include "stackframe.h"

namespace X
{
	namespace AST
	{
		class NamespaceVar;
	}
	namespace Data
	{
		class NamespaceVarObject :
			public virtual Object,
			public virtual AST::Scope
		{
			AST::StackFrame* m_stackFrame = nullptr;
		public:
			NamespaceVarObject()
			{
				m_stackFrame = new AST::StackFrame(this);
			}
			~NamespaceVarObject()
			{
				if (m_stackFrame)
				{
					delete m_stackFrame;
				}
			}
			virtual void GetBaseScopes(std::vector<AST::Scope*>& bases);
			inline virtual bool Get(XlangRuntime* rt, XObj* pContext,
				int idx, X::Value& v, LValue* lValue = nullptr)
			{
				m_stackFrame->Get(idx, v, lValue);
				return true;
			}
			bool Get(int idx, X::Value& v, LValue* lValue)
			{
				m_stackFrame->Get(idx, v, lValue);
				return true;
			}
			void AddSlotTo(int index)
			{
				if (index >= m_stackFrame->GetVarCount())
				{
					m_stackFrame->SetVarCount(index + 1);
				}
			}
			void Set(int index, X::Value& val)
			{
				m_stackFrame->Set(index, val);
			}

			// Inherited via Scope
			virtual Scope* GetParentScope() override { return nullptr; }
		};
	}
}
