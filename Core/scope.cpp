#include "scope.h"
#include "module.h"
#include "var.h"

namespace X
{
	namespace AST
	{
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