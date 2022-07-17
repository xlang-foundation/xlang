#include "package.h"
#include "manager.h"
#include "pyproxyobject.h"

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
			bool bOK = Manager::I().QueryAndCreatePackage(rt,strName, &pPackage);
			if (bOK)
			{
				v = Value(pPackage);
			}
			else
			{
				std::string path = rt->M()->GetModulePath();
				auto* pProxyObj = 
					new Data::PyProxyObject(rt,pContext,strName, path);
				v = Value(pProxyObj);
			}
			rt->M()->Add(rt, strName, nullptr, v);
		}
	}
	return true;
}
