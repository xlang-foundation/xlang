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
			FORCE_INLINE virtual bool Get(XlangRuntime* rt, XObj* pContext,
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
