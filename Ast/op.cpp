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

namespace X
{
namespace AST
{
	bool Assign::Run(XlangRuntime* rt, XObj* pContext, Value& v, LValue* lValue)
	{
		if (!L || !R)
		{
			v = Value(false);
			return false;
		}
		Value v_l;
		LValue lValue_L = nullptr;
		L->Run(rt, pContext, v_l, &lValue_L);
		Value v_r;
		if (!R->Run(rt, pContext, v_r))
		{
			v = Value(false);
			return false;
		}
		bool bOK = true;
		if (v_l.IsObject())
		{
			auto* pObj = v_l.GetObj();
			if (pObj->GetType() == X::ObjType::FuncCalls)
			{
				auto* pCalls = dynamic_cast<Data::FuncCalls*>(pObj);
				bOK =  pCalls->SetValue(v_r);
				v = Value(bOK);
				return bOK;
			}
			else if (pObj->GetType() == X::ObjType::Prop)
			{
				auto* pPropObj = dynamic_cast<Data::PropObject*>(pObj);
				bOK = pPropObj->SetPropValue(rt, lValue_L.GetContext(), v_r);
				v = Value(bOK);
				return bOK;
			}
			else if (pObj->GetType() == X::ObjType::RemoteObject)
			{
				auto* pRemoteObj = dynamic_cast<X::RemoteObject*>(pObj);
				bOK = pRemoteObj->SetValue(rt, pContext,v_r);
				v = Value(bOK);
				return bOK;
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
				//TODO: need clone??
				//lValue_L->Clone();
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
				bOK = L->Set(rt, pContext, v_r);
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
		v = Value(bOK);
		return bOK;
	}

	bool Operator::GetParamList(XlangRuntime* rt, Expression* e, ARGS& params, KWARGS& kwParams)
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

bool UnaryOp::Run(XlangRuntime* rt,XObj* pContext,Value& v,LValue* lValue)
{
	Value v_r;
	if (!R->Run(rt,pContext,v_r))
	{
		return false;
	}
	auto func = G::I().R().OpAct(Op).unaryop;
	return func ? func(rt,this, v_r, v) : false;
}

bool Range::Eval(XlangRuntime* rt)
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
bool Range::Run(XlangRuntime* rt, XObj* pContext, Value& v, LValue* lValue)
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
bool InOp::Run(XlangRuntime* rt, XObj* pContext, Value& v, LValue* lValue)
{
	if (v.IsInvalid())
	{
		//todo: deal with range()
		auto* pIt = new Data::Iterator();
		Value  varContainer;
		bool bOK = R->Run(rt, pContext, varContainer);
		if (bOK)
		{
			pIt->SetContainer(varContainer);
			if (L && L->m_type == ObType::Var)
			{
				pIt->SetImpactVar(dynamic_cast<Var*>(L));
			}
		}
		v = X::Value(dynamic_cast<XObj*>(pIt));
	}
	return true;
}
bool ColonOP::OpWithOperands(std::stack<AST::Expression*>& operands)
{
	//for right operands, support multiple token
	//for example: x: long int, the type is two-tokens word
	//so pop up all operands which's tokenIndex>op's token index
	AST::Expression* operandR = nullptr;
	while (!operands.empty() 
		&& operands.top()->GetTokenIndex() > m_tokenIndex)
	{
		auto r = operands.top();
		operands.pop();
		if (r->m_type != ObType::Var)
		{//only accept Var, all other like """comment""" skiped
			delete r;
			continue;
		}
		if (r->m_type == ObType::Var
			&& operandR!= nullptr 
			&& operandR->m_type == ObType::Var)
		{
			dynamic_cast<AST::Var*>(operandR)->MergeWithPreviousToken(
				dynamic_cast<AST::Var*>(r));
			delete r;
		}
		else if (operandR == nullptr)
		{//must be Var
			operandR = r;
		}
	}
	if (operandR == nullptr)
	{
		std::cout << "syntax error" << std::endl;
		return false;
	}
	auto operandL = operands.top();
	if (operandL == nullptr)
	{
		std::cout << "syntax error" << std::endl;
		return false;
	}
	operands.pop();
	auto param = new AST::Param(operandL, operandR);
	param->SetTokenIndex(operandL->GetTokenIndex());
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
			list->SetTokenIndex(operandL->GetTokenIndex());
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