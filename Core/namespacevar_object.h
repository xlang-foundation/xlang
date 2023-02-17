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
			public virtual Object
		{
			AST::StackFrame* m_stackFrame = nullptr;
			AST::NamespaceVar* m_nmVar = nullptr;
		public:
			NamespaceVarObject(AST::NamespaceVar* nmVar)
			{
				m_nmVar = nmVar;
				m_stackFrame = new AST::StackFrame((AST::Scope*)m_nmVar);
			}
			~NamespaceVarObject()
			{
				if (m_stackFrame)
				{
					delete m_stackFrame;
				}
			}
			virtual void GetBaseScopes(std::vector<AST::Scope*>& bases);
			bool Get(int idx, X::Value& v, LValue* lValue)
			{
				m_stackFrame->Get(idx, v, lValue);
				return true;
			}
			void Set(int index, X::Value& val)
			{
				if (index >= m_stackFrame->GetVarCount())
				{
					m_stackFrame->SetVarCount(index + 1);
				}
				m_stackFrame->Set(index, val);
			}
		};
	}
}
