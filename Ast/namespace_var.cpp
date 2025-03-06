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

#include "namespace_var.h"
#include "dotop.h"

namespace X
{
	namespace AST
	{
		void NamespaceVar::Add(Expression* item)
		{
			Block::Add(item);//hold this item into block for releasing this item
			return;
			switch (item->m_type)
			{
			case ObType::Assign:
			{
				auto* op = dynamic_cast<BinaryOp*>(item);
				auto* left = op->GetL();
				if (left->m_type == ObType::Var)
				{
					std::string name = (dynamic_cast<Var*>(left))->GetNameString();
					int index = 0;//todo: AddOrGet(name, false);
					auto* right = op->GetR();
					if (right)
					{
						ExecAction action;
						X::Value valRight;
						bool bOK = ExpExec(right,nullptr, action, nullptr, valRight);
						if (bOK)
						{
							//Obj()->Set(index, valRight);
						}
					}
				}
			}//ObType::Assign
				break;
			case ObType::NamespaceVar:
			{
				auto* pItem = dynamic_cast<NamespaceVar*>(item);
				std::string itemName = pItem->GetName();
				int index = 0;// AddOrGet(itemName, false);
				X::Value valRight;// = pItem->GetVal();
				//Obj()->Set(index, valRight);
			}//ObType::NamespaceVar
				break;
			default:
				break;
			}
		}
		void NamespaceVar::AddHhierarchy(Expression* item)
		{
			if (item->m_type == ObType::Var)
			{
				std::string itemName = dynamic_cast<Var*>(item)->GetNameString();
				int index = 0;// AddOrGet(itemName, false);
				X::Value valRight;
				//Obj()->Set(index, valRight);
			}
		}
		// Helper function to recursively get the leftmost Var name
		std::string NamespaceVar::GetLeftmostVarName(Expression* obj)
		{
			if (!obj)
				return "";

			if (obj->m_type == ObType::Var)
			{
				return dynamic_cast<Var*>(obj)->GetNameString();
			}
			else if (obj->m_type == ObType::Dot)
			{
				return GetLeftmostVarName(dynamic_cast<DotOp*>(obj)->GetL());
			}

			return ""; // Handle other types if needed
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
				else if (R->m_type == ObType::Dot)
				{
					strName = GetLeftmostVarName(dynamic_cast<DotOp*>(R)->GetL());
				}
			}

			Expression* pFromExp = this;
			while (pMyScope != nullptr && idx < 0)
			{
				SCOPE_FAST_CALL_AddOrGet0_NoDef(idx,pMyScope,strName, false);
				if (idx >= 0)
				{//use the scope to find this name as its scope
					m_scope = pMyScope;
					break;
				}
				//Find next upper real scope
				pMyScope = nullptr;
				Expression* pa = pFromExp->GetParent();
				while (pa != nullptr)
				{
					pMyScope = pa->GetMyScope();
					if (pMyScope)
					{
						//save for next loop
						pFromExp = pa;
						break;
					}
					pa = pa->GetParent();
				}
			}
			m_Index = idx;
		}
		bool NamespaceVar::Exec(XlangRuntime* rt,
			ExecAction& action, XObj* pContext,
			Value& v, LValue* lValue)
		{
			if (m_obj.IsInvalid())
			{
				if (m_Index == -1)
				{
					ScopeLayout();
				}
				Scope* pScope = GetScope();
				pScope->Get(rt,pContext, m_Index, v, lValue);
				if (v.IsInvalid())
				{
					v = new Data::NamespaceVarObject();
					m_obj = v;
					pScope->Set(rt, pContext, m_Index, m_obj);
				}
				else
				{
					m_obj = v;
				}
			}
			bool bOK = Block::Exec(rt,action,pContext,v,lValue);
			//Set(rt, pContext, m_valObj);//Set Scope Value with m_Index
			//for Hhierarchy, need to change v;
			if(R->m_type == ObType::Dot)
			{
				auto* dotOp = dynamic_cast<DotOp*>(R);
				auto* r0 = dotOp->GetR();
				if (r0->m_type == ObType::Var)
				{
					auto* ro_var = dynamic_cast<Var*>(r0);
					//TODO: Scope Issue
					ro_var->SetScope(dynamic_cast<Scope*>(this));
					ro_var->SetIsLeftValue(true);
					X::Value defVal;
					//take a place
					ro_var->Set(rt, pContext, defVal);
					ExecAction action0;
					bOK = ExpExec(r0,rt, action0,pContext, v, lValue);
				}
				else if (r0->m_type == ObType::Dot)
				{
					auto* r0_dot = dynamic_cast<DotOp*>(r0);
					auto* pChildNamespaceVar = new NamespaceVar(getOp());
					pChildNamespaceVar->SetR(r0);
					//pChildNamespaceVar->SetScope(this);
					Block::Add(pChildNamespaceVar);//hold this item into block for releasing this item
					ExecAction action0;
					bOK = ExpExec(pChildNamespaceVar,rt, action0, pContext, v, lValue);
					pChildNamespaceVar->SetR(nullptr); // don't let be deleted twice
				}
			}
			else
			{
				//v = m_valObj;
			}
			return bOK;
		}
	}
}