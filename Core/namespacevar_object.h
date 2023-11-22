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
		class Scope;
	}
	namespace Data
	{
		class NamespaceVarObject :
			public virtual Object
		{
			AST::Scope* m_pMyScope = nullptr;
			AST::StackFrame* m_variableFrame = nullptr;
		public:
			NamespaceVarObject()
			{
				m_pMyScope = new AST::Scope();
				m_variableFrame = new AST::StackFrame();
				m_pMyScope->SetVarFrame(m_variableFrame);
			}
			~NamespaceVarObject()
			{
				delete m_pMyScope;
				delete m_variableFrame;
			}
			virtual void GetBaseScopes(std::vector<AST::Scope*>& bases);
			inline virtual bool Get(XlangRuntime* rt, XObj* pContext,
				int idx, X::Value& v, LValue* lValue = nullptr)
			{
				m_variableFrame->Get(idx, v, lValue);
				return true;
			}
			bool Get(int idx, X::Value& v, LValue* lValue)
			{
				m_variableFrame->Get(idx, v, lValue);
				return true;
			}
			void AddSlotTo(int index)
			{
				if (index >= m_variableFrame->GetVarCount())
				{
					m_variableFrame->SetVarCount(index + 1);
				}
			}
			void Set(int index, X::Value& val)
			{
				m_variableFrame->Set(index, val);
			}
		};
	}
}
