#include "op.h"
#include "var.h"
#include "object.h"
#include "def.h"
#include "dict.h"
#include "pair.h"
#include "prop.h"
#include "funclist.h"
#include "op_registry.h"

namespace X
{
namespace AST
{
	bool Assign::Run(Runtime* rt, XObj* pContext, Value& v, LValue* lValue)
	{
		if (!L || !R)
		{
			return false;
		}
		Value v_l;
		LValue lValue_L = nullptr;
		L->Run(rt, pContext, v_l, &lValue_L);
		Value v_r;
		if (!R->Run(rt, pContext, v_r))
		{
			return false;
		}
		if (v_l.IsObject())
		{
			auto* pObj = v_l.GetObj();
			if (pObj->GetType() == X::ObjType::FuncCalls)
			{
				auto* pCalls = dynamic_cast<Data::FuncCalls*>(pObj);
				return pCalls->SetValue(v_r);
			}
			else if (pObj->GetType() == X::ObjType::Prop)
			{
				auto* pPropObj = dynamic_cast<Data::PropObject*>(pObj);
				return pPropObj->SetProp(rt, lValue_L.GetContext(), v_r);
			}
		}
		if (lValue_L)
		{
			switch (opId)
			{
			case X::OP_ID::Equ:
				*lValue_L = v_r;
				break;
			case X::OP_ID::AddEqu:
				lValue_L->Clone();
				*lValue_L += v_r;
				break;
			case X::OP_ID::MinusEqu:
				break;
			case X::OP_ID::MulEqu:
				*lValue_L *= v_r;
				break;
			case X::OP_ID::DivEqu:
				break;
			case X::OP_ID::ModEqu:
				break;
			case X::OP_ID::FloorDivEqu:
				break;
			case X::OP_ID::PowerEqu:
				break;
			case X::OP_ID::AndEqu:
				break;
			case X::OP_ID::OrEqu:
				break;
			case X::OP_ID::NotEqu:
				break;
			case X::OP_ID::RightShiftEqu:
				break;
			case X::OP_ID::LeftShitEqu:
				break;
			case X::OP_ID::Count:
				break;
			default:
				break;
			}
		}
		else
		{
			switch (opId)
			{
			case X::OP_ID::Equ:
				L->Set(rt, pContext, v_r);
				break;
			case X::OP_ID::AddEqu:
				v_l.Clone();
				v_l += v_r;
				break;
			case X::OP_ID::MinusEqu:
				break;
			case X::OP_ID::MulEqu:
				break;
			case X::OP_ID::DivEqu:
				break;
			case X::OP_ID::ModEqu:
				break;
			case X::OP_ID::FloorDivEqu:
				break;
			case X::OP_ID::PowerEqu:
				break;
			case X::OP_ID::AndEqu:
				break;
			case X::OP_ID::OrEqu:
				break;
			case X::OP_ID::NotEqu:
				break;
			case X::OP_ID::RightShiftEqu:
				break;
			case X::OP_ID::LeftShitEqu:
				break;
			case X::OP_ID::Count:
				break;
			default:
				break;
			}
		}
		return true;
	}

	bool Operator::GetParamList(Runtime* rt, Expression* e, ARGS& params, KWARGS& kwParams)
	{
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
				bOK = valExpr->Run(rt, nullptr, v0);
				if (bOK)
				{
					kwParams.emplace(std::make_pair(strVarName, v0));
				}
			}
			else
			{
				Value v0;
				bOK = i->Run(rt, nullptr, v0);
				if (bOK)
				{
					params.push_back(v0);
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
		return bOK;
	}

bool UnaryOp::Run(Runtime* rt,XObj* pContext,Value& v,LValue* lValue)
{
	Value v_r;
	if (!R->Run(rt,pContext,v_r))
	{
		return false;
	}
	auto func = G::I().R().OpAct(Op).unaryop;
	return func ? func(rt,this, v_r, v) : false;
}

bool Range::Eval(Runtime* rt)
{
	if (R && R->m_type == ObType::Pair)
	{
		PairOp* p = dynamic_cast<PairOp*>(R);
		Expression* param = p->GetR();
		if (param)
		{
			if (param->m_type == ObType::List)
			{

			}
			else
			{//only one parameter, means stop
				Value vStop;
				param->Run(rt, nullptr, vStop);
				if (vStop.GetType() == ValueType::Int64)
				{
					m_stop = vStop.GetLongLong();
				}
			}
		}

	}
	m_evaluated = true;
	return true;
}
bool Range::Run(Runtime* rt, XObj* pContext, Value& v, LValue* lValue)
{
	if (!m_evaluated)
	{
		Eval(rt);
	}
	if (v.GetType() != ValueType::Int64)
	{//not started
		v = Value(m_start);
	}
	else
	{
		v += m_step;
	}
	return (v.GetLongLong() < m_stop);
}
bool ColonOP::OpWithOperands(std::stack<AST::Expression*>& operands)
{
	auto operandR = operands.top();
	operands.pop();
	auto operandL = operands.top();
	operands.pop();
	auto param = new AST::Param(operandL, operandR);
	param->ReCalcHint(operandL);
	param->ReCalcHint(operandR);
	operands.push(param);
	return true;
}

bool CommaOp::OpWithOperands(std::stack<AST::Expression*>& operands)
{
	AST::List* list = nil;

	auto operandR = operands.top();
	operands.pop();

	//L may be not there
	if (!operands.empty())
	{
		auto operandL = operands.top();
		operands.pop();
		if (operandL->m_type != AST::ObType::List)
		{
			list = new AST::List(operandL);
		}
		else
		{
			list = dynamic_cast<AST::List*>(operandL);
		}
	}
	else
	{
		list = new AST::List();
	}
	if (operandR->m_type != AST::ObType::List)
	{
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

bool SemicolonOp::OpWithOperands(std::stack<AST::Expression*>& operands)
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