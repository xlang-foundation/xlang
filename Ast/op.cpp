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

#include "op.h"
#include "var.h"
#include "object.h"
#include "def.h"
#include "dict.h"
#include "pair.h"
#include "prop.h"
#include "funclist.h"
#include "op_registry.h"
#include "remote_object.h"
#include "iterator.h"
#include "struct.h"

namespace X
{
namespace AST
{
	bool InOp::Exec(XlangRuntime* rt, ExecAction& action, 
		XObj* pContext, Value& v, LValue* lValue)
	{
		bool bOK = true;
		if (L == nullptr || R == nullptr)
		{
			return false;
		}
		ExecAction action_getVal;
		X::Value valLeft;
		bOK = ExpExec(L, rt, action_getVal, pContext, valLeft);
		if (!bOK)
		{
			return false;
		}
		X::Value containerObj;
		bOK = ExpExec(R, rt, action_getVal, pContext, containerObj);
		if (!bOK)
		{
			return false;
		}
		if (containerObj.IsObject())
		{
			auto* pObj = dynamic_cast<X::Data::Object*>(containerObj.GetObj());
			v = pObj->IsContain(valLeft);
		}
		else
		{
			v = Value(false);
		}
		return bOK;
	}

	bool Assign::ObjectAssign(XlangRuntime* rt, XObj* pContext,XObj* pObj, Value& v, Value& v_r, LValue& lValue_L)
	{
		bool bOK = true;
		auto ty = pObj->GetType();
		switch (ty)
		{
		case X::ObjType::FuncCalls:
		{
			auto* pCalls = dynamic_cast<Data::FuncCalls*>(pObj);
			bOK = pCalls->SetValue(v_r);
			v = Value(bOK);
		}
		case X::ObjType::Prop:
		{
			auto* pPropObj = dynamic_cast<Data::PropObject*>(pObj);
			bOK = pPropObj->SetPropValue(rt, lValue_L.GetContext(), v_r);
			v = Value(bOK);
		}
		break;
		case X::ObjType::StructField:
		{
			auto* pXlangStructField = dynamic_cast<Data::XlangStructField*>(pObj);
			bOK = pXlangStructField->SetValue(rt, lValue_L.GetContext(), v_r);
			v = Value(bOK);
		}
		break;
		case X::ObjType::RemoteObject:
		{
			//if Left side is a remote object, and right side is not a
			//remote object, we treat as SetValue for this Left RemoteObject
			//but for both sides are remote objects,treat as assign operator.
			//TODO(Shawn 6/5/2023): But we need to consider more on this case.
			if (v_r.IsNone()) //for case: Set RemoteObject to None
			{
				bOK = false;
			}
			else if (!v_r.IsObject() || v_r.GetObj()->GetType() != X::ObjType::RemoteObject)
			{
				auto* pRemoteObj = dynamic_cast<X::RemoteObject*>(pObj);
				bOK = pRemoteObj->SetValue(rt, pContext, v_r);
				v = Value(bOK);
			}
		}
		default:
			bOK = false;
			break;
		}
		return bOK;
	}
	bool Operator::GetParamList(XlangRuntime* rt, Expression* e, ARGS& params, KWARGS& kwParams)
	{
		std::vector<X::Value> vecParam;
		std::vector<std::pair<std::string,X::Value>> vecKwParam;
		auto proc = [&](Expression* i)
		{
			bool bOK = true;
			if (i->m_type == ObType::Assign)
			{
				Assign* assign = dynamic_cast<Assign*>(i);
				Var* varName = dynamic_cast<Var*>(assign->GetL());
				String& szName = varName->GetName();
				std::string strVarName = std::string(szName.s, szName.size);
				Expression* valExpr = assign->GetR();
				Value v0;
				ExecAction action;
				bOK = ExpExec(valExpr,rt,action,nullptr, v0);
				if (bOK)
				{
					vecKwParam.push_back(std::make_pair(strVarName,v0));
				}
			}
			else
			{
				Value v0;
				ExecAction action;
				bOK = ExpExec(i,rt,action,nullptr, v0);
				if (bOK)
				{
					vecParam.push_back(v0);
				}
			}
			return bOK;
		};
		bool bOK = true;
		if (e->m_type != ObType::List)
		{
			bOK = proc(e);
		}
		else
		{
			auto& list = (dynamic_cast<List*>(e))->GetList();
			for (auto i : list)
			{
				bOK = proc(i);
				if (!bOK)
				{
					break;
				}
			}
		}
		if (vecParam.size() > 0)
		{
			params.resize(vecParam.size());
			for (auto& v : vecParam)
			{
				params.push_back(v);
			}
		}
		if (vecKwParam.size() > 0)
		{
			kwParams.resize(vecKwParam.size());
			for (auto& item : vecKwParam)
			{
				kwParams.Add(item.first.c_str(), item.second, true);
			}
		}
		return bOK;
	}

bool UnaryOp::Exec(XlangRuntime* rt,ExecAction& action,XObj* pContext,Value& v,LValue* lValue)
{
	//for case: return without value
	if (opId == OP_ID::ReturnOp && R == nullptr)
	{
		action.type = ExecActionType::Return;
		return true;
	}
	Value v_r;
	if (!ExpExec(R,rt,action,pContext,v_r))
	{
		return false;
	}
	auto func = G::I().R().OpAct(Op).unaryop;
	if (opId == OP_ID::ReturnOp)
	{
		action.type = ExecActionType::Return;
	}
	return func ? func(rt,this, v_r, v) : false;
}

bool ColonOP::OpWithOperands(std::stack<AST::Expression*>& operands, int LeftTokenIndex)
{
	//for right operands, support multiple token
	//for example: x: long int, the type is two-tokens word
	//so pop up all operands which's tokenIndex>op's token index
	std::vector<AST::Expression*> vecOperandR;
	while (!operands.empty() 
		&& operands.top()->GetTokenIndex() > m_tokenIndex)
	{
		vecOperandR.push_back(operands.top());
		operands.pop();
	}
	AST::Expression* operandR = nullptr;
	auto opernandRSize = vecOperandR.size();
	if (opernandRSize > 0)
	{
		operandR = vecOperandR[opernandRSize - 1];
		//we check if they are Var, just merge names to first one 
		//and just adjust first one's name's size to reach the end of last one
		if (operandR->m_type == AST::ObType::Var)
		{
			auto* var_r = dynamic_cast<AST::Var*>(operandR);
			auto& name_r = var_r->GetName();
			if (opernandRSize > 1)
			{
				auto* operand_first = vecOperandR[0];
				if (operand_first->m_type == AST::ObType::Var)
				{
					auto* var_first = dynamic_cast<AST::Var*>(operand_first);
					auto& name_first = var_first->GetName();
					name_r.size = name_first.s+name_first.size- name_r.s;
				}
			}
		}
		else
		{
			//TODO:
		}
	}

#if NOT_SUPPORT //we want to support 1:[skip] for tensor index,
	//but check if have some other impacts
	if (operandR == nullptr)
	{
		std::cout << "syntax error" << std::endl;
		return false;
	}
#endif
	AST::Expression* operandL = nullptr;
	while (!operands.empty()
		&& operands.top()->GetTokenIndex() > LeftTokenIndex)
	{
		operandL = operands.top();
		operands.pop();
	}

#if NOT_SUPPORT 
	if (operandL == nullptr)
	{
		std::cout << "syntax error" << std::endl;
		return false;
	}
#endif
	auto param = new AST::Param(operandL, operandR);
	if (operandL)
	{
		param->SetTokenIndex(operandL->GetTokenIndex());
		param->ReCalcHint(operandL);
	}
	else
	{//not set TokenIndex, use this op's
		param->SetTokenIndex(m_tokenIndex);
		//if no Left,use this ColonOP's Hints
		param->ReCalcHint(this);
	}
	operands.push(param);
	return true;
}

bool CommaOp::OpWithOperands(std::stack<AST::Expression*>& operands, int LeftTokenIndex)
{
	AST::List* list = nil;

	auto operandR = operands.top();
	operands.pop();

	//L may be not there
	if (!operands.empty())
	{
		auto operandL = operands.top();
		if (operandL->GetTokenIndex()> LeftTokenIndex)
		{
			operands.pop();
			if (operandL->m_type != AST::ObType::List)
			{
				list = new AST::List(operandL);
				list->SetTokenIndex(operandL->GetTokenIndex());
			}
			else
			{
				list = dynamic_cast<AST::List*>(operandL);
			}
		}
	}
	if(list == nil)
	{
		list = new AST::List();
	}
	if (operandR->m_type != AST::ObType::List)
	{
		if (list->GetTokenIndex() == -1)
		{
			list->SetTokenIndex(operandR->GetTokenIndex());
		}
		*list += operandR;
	}
	else
	{
		List& list_r = *dynamic_cast<AST::List*>(operandR);
		*list += list_r;
		list_r.ClearList();
		delete operandR;
	}
	operands.push(list);
	return true;
}

bool SemicolonOp::OpWithOperands(std::stack<AST::Expression*>& operands, int LeftTokenIndex)
{
	return true;
}
void ExternDecl::ScopeLayout()
{
	UnaryOp::ScopeLayout();
	Scope* myScope = GetScope();
	if (R->m_type != ObType::List)
	{
		myScope->AddExternVar(dynamic_cast<Var*>(R));
	}
	else
	{
		auto& list = (dynamic_cast<List*>(R))->GetList();
		for (auto i : list)
		{
			myScope->AddExternVar(dynamic_cast<Var*>(i));
		}
	}
}

}
}