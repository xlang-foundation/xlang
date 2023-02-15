#include "namespace_var.h"
#include "namespacevar_object.h"

namespace X
{
	namespace AST
	{
		void NamespaceVar::Add(Expression* item)
		{
			Scope* pMyScope = GetScope();
		}
		void NamespaceVar::ScopeLayout()
		{
			Scope* pMyScope = GetScope();
			int idx = -1;
			std::string strName;
			if (R)
			{
				if (R->m_type == ObType::Var)
				{
					strName = dynamic_cast<Var*>(R)->GetNameString();
				}
			}
			while (pMyScope != nullptr && idx < 0)
			{
				idx = pMyScope->AddOrGet(strName, false);
				if (idx >= 0)
				{//use the scope to find this name as its scope
					m_scope = pMyScope;
					break;
				}
				pMyScope = pMyScope->GetParentScope();
			}
			m_Index = idx;
		}
		bool NamespaceVar::Exec(XlangRuntime* rt,
			ExecAction& action, XObj* pContext,
			Value& v, LValue* lValue)
		{
			auto* pObj = new X::Data::NamespaceVarObject();
			v = X::Value(pObj);
			return true;
		}
	}
}