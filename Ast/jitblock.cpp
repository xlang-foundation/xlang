﻿/*
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

#include "jitblock.h"
#include "object.h"
#include "function.h"

namespace X
{
	namespace AST
	{
		void JitBlock::ScopeLayout()
		{
			Scope* pMyScope = GetScope();
			if (pMyScope)
			{
				std::string strName(m_Name.s, m_Name.size);
				SCOPE_FAST_CALL_AddOrGet0_NoDef(m_Index,pMyScope,strName, false);
			}
			//process parameters' default values
			if (Params)
			{
				auto& list = Params->GetList();
				//this case happened in lambda function
				for (auto i : list)
				{
					std::string strVarName;
					std::string strVarType;
					Value defaultValue;
					switch (i->m_type)
					{
					case ObType::Var:
					{
						Var* varName = dynamic_cast<Var*>(i);
						String& szName = varName->GetName();
						strVarName = std::string(szName.s, szName.size);
					}
					break;
					case ObType::Assign:
					{
						Assign* assign = dynamic_cast<Assign*>(i);
						Var* varName = dynamic_cast<Var*>(assign->GetL());
						String& szName = varName->GetName();
						strVarName = std::string(szName.s, szName.size);
						Expression* defVal = assign->GetR();
						auto* pExprForDefVal = new Data::Expr(defVal);
						defaultValue = Value(pExprForDefVal);
					}
					break;
					case ObType::Param:
					{
						Param* param = dynamic_cast<Param*>(i);
						param->Parse(strVarName, strVarType, defaultValue);
					}
					break;
					}
					SCOPE_FAST_CALL_AddOrGet0(idx,m_pMyScope,strVarName, false);
					m_IndexofParamList.push_back(idx);
				}
				Params->ScopeLayout();
			}
		}
		bool JitBlock::Exec(XlangRuntime* rt, ExecAction& action, XObj* pContext,
			Value& v, LValue* lValue)
		{
			//when build from stream, n_Index stored into Stream,but m_scope is not
			if (m_Index == -1 || m_scope == nullptr)
			{
				ScopeLayout();
				//for lambda function, no name, so skip this check
				if (m_Name.size > 0 && m_Index == -1)
				{
					return false;
				}
			}
			//AST::JitFunc* jitFunc = new AST::JitFunc(this);
			Data::Function* f = new Data::Function(this);
			Value v0(f);
			if (m_Index >= 0)
			{
				m_scope->Set(rt, pContext, m_Index, v0);
			}
			v = v0;
			return true;
		}

	}
}