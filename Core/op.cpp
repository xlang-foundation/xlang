#include "op.h"
#include "var.h"
#include "object.h"
#include "def.h"
#include "dict.h"
#include "pair.h"
#include "funclist.h"

namespace X
{
namespace AST
{
bool UnaryOp::Run(Runtime* rt,void* pContext,Value& v,LValue* lValue)
{
	Value v_r;
	if (!R->Run(rt,pContext,v_r))
	{
		return false;
	}
	auto func = G::I().OpAct(Op).unaryop;
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
bool Range::Run(Runtime* rt, void* pContext, Value& v, LValue* lValue)
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
	param->SetHint(MIN_VAL(operandL->GetStartLine(),
		operandR->GetStartLine()),
		MIN_VAL(operandL->GetEndLine(),
			operandR->GetEndLine()),
		MIN_VAL(operandL->GetCharPos(),
			operandR->GetCharPos())
	);
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
			list = (AST::List*)operandL;
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
		List& list_r = *(AST::List*)operandR;
		*list += list_r;
		list_r.ClearList();
		delete operandR;
	}
	operands.push(list);
	return true;
}

bool Assign::AssignToDataObject(Runtime* rt, void* pObjPtr)
{
	Data::Object* pObj = (Data::Object*)pObjPtr;
	Value v_r;
	if (!R->Run(rt, nullptr, v_r))
	{
		return false;
	}
	if (pObj->GetType() == Data::Type::FuncCalls)
	{
		Data::FuncCalls* pCalls = dynamic_cast<Data::FuncCalls*>(pObj);
		return pCalls->SetValue(v_r);
	}
	else
	{
		return false;
	}
}

bool SemicolonOp::OpWithOperands(std::stack<AST::Expression*>& operands)
{
	return true;
}

}
}