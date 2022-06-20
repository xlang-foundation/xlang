#include "package.h"
#include "manager.h"

bool X::AST::Import::Run(Runtime* rt, void* pContext, 
	Value& v, LValue* lValue)
{
	if (R)
	{
		if (R->m_type == ObType::List)
		{

		}
		else if (R->m_type == ObType::Var)
		{
			Var* var = (Var*)R;
			String& name = var->GetName();
			std::string strName(name.s, name.size);
			Package* pPackage = nullptr;
			Manager::I().QueryAndCreatePackage(rt,strName, &pPackage);
			v = Value(pPackage);
			rt->M()->Add(rt, strName, nullptr, v);
		}
	}
	return true;
}
