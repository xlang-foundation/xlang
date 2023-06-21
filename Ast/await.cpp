#include "await.h"
#include "object.h"

namespace X
{
	namespace AST
	{
		bool AwaitOp::Exec(XlangRuntime* rt, ExecAction& action, 
			XObj* pContext, Value& v, LValue* lValue)
		{
			bool bWaited = true;
			if (R)
			{
				if (R->m_type == ObType::Var)
				{
					Value  var0;
					ExecAction action0;
					bool bOK = R->Exec(rt, action0, pContext, var0);
					if (bOK && var0.IsObject())
					{
						Data::Object* pObj = dynamic_cast<Data::Object*>(var0.GetObj());
						bWaited = pObj->wait(-1);// always wait
					}
				}
				else if (R->m_type == ObType::List)
				{
					AST::List* pList = dynamic_cast<AST::List*>(R);
					auto& list = pList->GetList();
					//wait one by one
					for (auto& exp : list)
					{
						if (exp->m_type == ObType::Var)
						{
							Value  var0;
							ExecAction action0;
							bool bOK = exp->Exec(rt, action0, pContext, var0);
							if (bOK)
							{
								Data::Object* pObj = dynamic_cast<Data::Object*>(var0.GetObj());
								bWaited = pObj->wait(-1);
							}
						}
					}
				}
				else
				{
					Value  var0;
					ExecAction action0;
					bool bOK = R->Exec(rt, action0, pContext, var0);
					if (bOK)
					{
						Data::Object* pObj = dynamic_cast<Data::Object*>(var0.GetObj());
						bWaited = pObj->wait(-1);
					}
				}
			}
			return bWaited;
		}
	}
}