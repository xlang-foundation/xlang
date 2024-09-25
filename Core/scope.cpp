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

#include "scope.h"
#include "module.h"
#include "var.h"

namespace X
{
	namespace AST
	{
		bool Scope::ToBytes(XlangRuntime* rt, XObj* pContext, X::XLangStream& stream)
		{
			stream << (int)m_Vars.size();
			for (auto& it : m_Vars)
			{
				stream << it.first;
				stream << it.second;
			}
			stream << (int)m_ExternVarMap.size();
			for (auto& it : m_ExternVarMap)
			{
				stream << it.first;
				it.second->ToBytes(rt,pContext,stream);
			}

			return true;
		}
		bool Scope::FromBytes(X::XLangStream& stream)
		{
			int varCount = 0;
			stream >> varCount;
			for (int i = 0; i < varCount; i++)
			{
				std::string name;
				int idx;
				stream >> name;
				stream >> idx;
				m_Vars.emplace(std::make_pair(name,idx));
			}
			int externVarCount = 0;
			stream >> externVarCount;
			for (int i = 0; i < externVarCount; i++)
			{
				std::string name;
				Var* pVar = new Var();
				stream >> name;
				pVar->FromBytes(stream);
				m_ExternVarMap.emplace(std::make_pair(name, pVar));
			}
			return true;
		}
		void Scope::AddExternVar(AST::Var* var)
		{
			std::string name = var->GetNameString();
			if (m_ExternVarMap.find(name) == m_ExternVarMap.end())
			{
				m_ExternVarMap.emplace(std::make_pair(name, var));
			}
		}
	}
}