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
					bool bOK = ExpExec(R,rt, action0, pContext, var0);
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
							bool bOK = ExpExec(exp,rt, action0, pContext, var0);
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
					bool bOK = ExpExec(R,rt, action0, pContext, var0);
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