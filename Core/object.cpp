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

#include "object.h"
#include "list.h"
#include "dict.h"
#include "xclass.h"
#include "attribute.h"

namespace X
{
	namespace Data
	{
		// Initialize the static atomic counter
		std::atomic<unsigned long long> Object::s_idCounter{ 0 };

		AttributeBag* Object::GetAttrBag()
		{
			AutoLock autoLock(m_lock);
			if (m_aBag == nullptr)
			{
				m_aBag = new AttributeBag();
			}
			return m_aBag;
		}
		void Object::DeleteAttrBag()
		{
			AutoLock autoLock(m_lock);
			if (m_aBag)
			{
				delete m_aBag;
				m_aBag = nullptr;
			}
		}
		X::Value Expr::ToKV()
		{
			if (m_expr == nullptr)
			{
				return X::Value();
			}
			X::Value retVal;
			auto expType = m_expr->m_type;
			switch (expType)
			{
			case X::AST::ObType::Assign:
			{
				std::string name;
				X::Value val;
				AST::Assign* pAssign = static_cast<AST::Assign*>(m_expr);
				if (pAssign->GetL())
				{
					auto* l = pAssign->GetL();
					if (l->m_type == X::AST::ObType::Var)
					{
						auto* pVar  = static_cast<AST::Var*>(l);
						name = pVar->GetNameString();
					}
				}
				if (pAssign->GetR())
				{
					AST::ExecAction action;		
					ExpExec(pAssign->GetR(), (XlangRuntime*)m_rt, action, this, val);
				}
				if (!name.empty())
				{
					X::Dict dict;
					dict->Set(name, val);
					retVal = dict;
				}
			}
				break;
			case X::AST::ObType::Param:
			{
				std::string name;
				X::Value val;
				AST::Param* pParam = static_cast<AST::Param*>(m_expr);
				if (pParam->GetName())
				{
					auto* l = pParam->GetName();
					if (l->m_type == X::AST::ObType::Var)
					{
						auto* pVar = static_cast<AST::Var*>(l);
						name = pVar->GetNameString();
					}
				}
				if (pParam->GetType())
				{
					AST::ExecAction action;
					ExpExec(pParam->GetType(), (XlangRuntime*)&m_rt, action, this, val);
				}
				if (!name.empty())
				{
					X::Dict dict;
					dict->Set(name, val);
					retVal = dict;
				}
			}
				break;
			default:
				break;
			}
			return retVal;
		}
	}
}