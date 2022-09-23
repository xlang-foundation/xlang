#include "scope.h"
#include "module.h"
#include "var.h"

namespace X
{
	namespace AST
	{
		bool Scope::ToBytes(Runtime* rt, XObj* pContext, X::XLangStream& stream)
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
		std::string Scope::GetModuleName(Runtime* rt)
		{
			return rt->M()->GetModuleName();
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