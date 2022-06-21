#include "op.h"
#include "var.h"
#include "object.h"
#include "def.h"
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

bool PairOp::GetParamList(Runtime* rt,Expression* e,
	std::vector<Value>& params,
	std::unordered_map<std::string, Value>& kwParams)
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
			bOK = valExpr->Run(rt,nullptr,v0);
			if (bOK)
			{
				kwParams.emplace(std::make_pair(strVarName, v0));
			}
		}
		else
		{
			Value v0;
			bOK = i->Run(rt,nullptr,v0);
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
		auto& list = ((List*)e)->GetList();
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
bool PairOp::ParentRun(Runtime* rt, void* pContext, Value& v, LValue* lValue)
{
	bool bOK = false;
	if (L)
	{//Call Func
		Value lVal;
		bOK = L->Run(rt, pContext, lVal, lValue);
		if (!bOK || !lVal.IsObject())
		{
			return bOK;
		}
		std::vector<Value> params;
		std::unordered_map<std::string, Value> kwParams;
		if (R)
		{
			bOK = GetParamList(rt, R, params, kwParams);
			if (!bOK)
			{
				return bOK;
			}
		}
		Data::Object* obj = (Data::Object*)lVal.GetObject();
		if (obj)
		{
			bOK = obj->Call(rt, params, kwParams, v);
		}
	}
	else
	{
		if (R && R->m_type != ObType::List)
		{
			bOK = R->Run(rt, pContext, v, lValue);
		}
	}
	return bOK;
}
bool PairOp::BracketRun(Runtime* rt, void* pContext, Value& v, LValue* lValue)
{
	bool bOK = false;
	if (L)
	{//usage: x[1,2]
		Value v0;
		bOK = L->Run(rt, pContext, v0);
		Data::List* pDataList = (Data::List*)v0.GetObject();
		//Get Index
		std::vector<long long> IdxAry;
		if (R->m_type == ObType::List)
		{
			auto& list = ((List*)R)->GetList();
			for (auto e : list)
			{
				Value v1;
				if (e->Run(rt, pContext, v1))
				{
					IdxAry.push_back(v1.GetLongLong());
				}
				else
				{
					bOK = false;
					break;
				}
			}
		}
		else
		{
			Value vIdx;
			bOK = R->Run(rt, pContext, vIdx);
			IdxAry.push_back(vIdx.GetLongLong());
		}
		if (bOK)
		{
			if (IdxAry.size() > 0)
			{
				pDataList->Get(IdxAry[0], v, lValue);
			}
		}
	}
	else
	{//Create list with []
		bOK = true;
		Data::List* pDataList = new Data::List();
		if (R && R->m_type == ObType::List)
		{
			auto& list = ((List*)R)->GetList();
			for (auto e : list)
			{
				Value v;
				if (e->Run(rt, pContext, v))
				{
					pDataList->Add(rt, v);
				}
				else
				{
					bOK = false;
					break;
				}
			}
		}
		else if (R && R->m_type == ObType::Var)
		{
			Value v;
			if (R->Run(rt, pContext, v))
			{
				pDataList->Add(rt, v);
			}
			else
			{
				bOK = false;
			}
		}
		v = Value(pDataList);
	}
	return bOK;
}
bool PairOp::CurlyBracketRun(Runtime* rt, void* pContext, Value& v, LValue* lValue)
{
	bool bOK = false;

	return bOK;
}
bool PairOp::Run(Runtime* rt,void* pContext,Value& v,LValue* lValue)
{
	bool bOK = false;
	if (Op == G::I().GetOpId(OP_ID::Parenthesis_L))
	{	
		bOK = ParentRun(rt, pContext, v, lValue);
	}
	else if (Op == G::I().GetOpId(OP_ID::Brackets_L))
	{
		bOK = BracketRun(rt, pContext, v, lValue);
	}
	else if (Op == G::I().GetOpId(OP_ID::Curlybracket_L))
	{
		bOK = CurlyBracketRun(rt, pContext, v, lValue);
	}
	return bOK;
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
	auto operandR = operands.top();
	operands.pop();
	auto operandL = operands.top();
	operands.pop();
	AST::List* list = nil;
	if (operandL->m_type != AST::ObType::List)
	{
		list = new AST::List(operandL);
	}
	else
	{
		list = (AST::List*)operandL;
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