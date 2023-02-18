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
						bool bOK = right->Exec(nullptr, action, nullptr, valRight);
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
					auto* dotOp = dynamic_cast<DotOp*>(R);
					strName = dynamic_cast<Var*>(dotOp->GetL())->GetNameString();
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
					ro_var->SetScope(dynamic_cast<Scope*>(this));
					ro_var->SetIsLeftValue(true);
					X::Value defVal;
					//take a place
					ro_var->Set(rt, pContext, defVal);
					ExecAction action0;
					bOK = r0->Exec(rt, action0,pContext, v, lValue);
				}
				else if (r0->m_type == ObType::Dot)
				{
					auto* r0_dot = dynamic_cast<DotOp*>(r0);
					auto* pChildNamespaceVar = new NamespaceVar(getOp());
					pChildNamespaceVar->SetR(r0);
					//pChildNamespaceVar->SetScope(this);
					Block::Add(pChildNamespaceVar);//hold this item into block for releasing this item
					ExecAction action0;
					bOK = pChildNamespaceVar->Exec(rt, action0, pContext, v, lValue);
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