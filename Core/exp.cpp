#include "exp.h"
#include "builtin.h"
#include <iostream>

namespace XPython {namespace AST {

Scope* Expression::FindScope()
{
	Scope* pMyScope = nil;
	Expression* pa = m_parent;
	while (pa != nil && pMyScope == nil)
	{
		pMyScope = dynamic_cast<Scope*>(pa);
		pa = pa->GetParent();
	}
	return pMyScope;
}

Func* Expression::FindFuncByName(Var* name)
{
	Func* pFuncRet = nil;
	Expression* pa = m_parent;
	while (pa != nil)
	{
		Block* pMyBlock = dynamic_cast<AST::Block*>(pa);
		if (pMyBlock)
		{
			pFuncRet = pMyBlock->FindFuncByName(name);
			if (pFuncRet)
			{
				break;
			}
		}
		pa = pa->GetParent();
	}
	return pFuncRet;
}

void Var::Set(Value& v)
{
	auto pScope = FindScope();
	if (pScope)
	{
		std::string key(Name.s, Name.size);
		pScope->Set(key, v);
	}
}

bool Var::Run(Value& v)
{
	bool bOK = false;
	std::string key(Name.s, Name.size);
	auto pScope = FindScope();
	if (pScope)
	{
		bOK = pScope->Get(key, v);
	}
	return bOK;
}

bool UnaryOp::Run(Value& v)
{
	Value v_r;
	if (!R->Run(v_r))
	{
		return false;
	}
	auto func = G::I().OpAct(Op).unaryop;
	return func ? func(this, v_r, v) : false;
}
bool Func::SetParamsIntoFrame(StackFrame* frame, List* param_values)
{
	if (Params == nil)
	{
		return true;//no parameters required
	}
	auto param_def = Params->GetList();
	auto values = param_values->GetList();
	for (int i=0;i<(int)param_def.size();i++)
	{
		Param* p = dynamic_cast<Param*>(param_def[i]);
		if (p)
		{
			Var* name = dynamic_cast<Var*>(p->GetName());
			//TODO: type check
			String& sName = name->GetName();
			std::string strName(sName.s, sName.size);
			Value v;
			if (i < (int)values.size())
			{
				Expression* val = values[i];
				if (val)
				{
					val->Run(v);
				}
			}
			frame->Set(strName, v);
		}
	}
	return true;
}
bool Func::Call(List* params, Value& retValue)
{
	StackFrame* frame = new StackFrame();
	SetParamsIntoFrame(frame, params);
	PushFrame(frame);
	Value v0;
	Block::Run(v0);
	PopFrame();
	retValue = frame->GetReturnValue();
	delete frame;
	return true;
}

bool PairOp::Run(Value& v)
{
	bool bOK = false;
	if (Op == G::I().GetOpId(OP_ID::Parenthesis_L))
	{//Call Func
		if (L && L->m_type == ObType::Var)
		{
			Func* pFunc = FindFuncByName((Var*)L);
			if (pFunc)
			{
				Value retValue;
				List* inParam = nil;
				if (R->m_type != ObType::List)
				{
					inParam = new List(R);
					inParam->SetParent(this);
				}
				else
				{
					inParam = (List*)R;
				}
				if (pFunc->Call(inParam, retValue))
				{//v is return value if changed
					v = retValue;
					bOK = true;
				}
			}
		}
		else if(R)
		{//like (x+1), just need to use R to eval
			bOK = R->Run(v);
		}
	}
	return bOK;
}
void Block::Add(Expression* item)
{
	if (Body.size() > 0)
	{//for if elif else 
		Expression* pLastExp = Body[Body.size() - 1];
		if (pLastExp->EatMe(item))
		{
			return;
		}
	}
	Body.push_back(item);
	if (item) item->SetParent(this);
}
Func* Block::FindFuncByName(Var* name)
{
	Func* func = nil;
	String& target_name = name->GetName();
	//internl first
	std::string strName(target_name.s, target_name.size);
	ExternFunc* extFunc = Builtin::I().Find(strName);
	if (extFunc)
	{
		return extFunc;
	}
	for (auto i : Body)
	{
		if (i->m_type != ObType::Func)
		{
			continue;
		}
		auto iFunc = (Func*)i;
		Var* ivarName = dynamic_cast<Var*>(iFunc->GetName());
		if (ivarName == nil)
		{
			continue;
		}
		String& i_name = ivarName->GetName();
		bool bMatched = false;
		if (i_name.size == target_name.size && i_name.size>0)
		{
			bMatched = true;
			for (int j = 0; j < i_name.size; j++)
			{
				if (i_name.s[j] != target_name.s[j])
				{
					bMatched = false;
					break;
				}
			}
		}
		if (bMatched)
		{
			func = iFunc;
			break;
		}
	}
	return func;
}

bool ExternFunc::Call(List* params, Value& retValue)
{
	//DO PRINT first
	if (params)
	{
		auto values = params->GetList();
		for (int i = 0; i < (int)values.size(); i++)
		{
			Expression* exp = values[i];
			Value v;
			exp->Run(v);
			std::string str = v.ToString();
			std::cout << str;
		}
		std::cout << std::endl;
	}
	return true;
}
bool While::Run(Value& v)
{
	if (R == nil)
	{
		return false;
	}
	Value v0;
	while (true)
	{
		Value v0;
		bool bOK = R->Run(v0);
		if (bOK && v0 == Value(true))
		{
			Block::Run(v);
		}
		else
		{
			break;
		}
	}
	return true;
}
bool For::Run(Value& v)
{
	Value v0;
	while (true)
	{
		if (R)
		{
			bool bC0 = R->Run(v0);
			if (!bC0)
			{
				break;
			}
		}
		Block::Run(v);
	}
	return true;
}

bool InOp::Run(Value& v)
{
	bool bIn = false;
	if (R)
	{
		if (R->m_type == ObType::Range)
		{
			Range* r = dynamic_cast<Range*>(R);
			bIn = r->Run(v);//will update v
			if (bIn)
			{
				L->Set(v);
			}
		}
	}
	return bIn;
}

bool Range::Eval()
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
				param->Run(vStop);
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
bool Range::Run(Value& v)
{
	if (!m_evaluated)
	{
		Eval();
	}
	if (v.GetType() != ValueType::Int64)
	{//not started
		v = Value(m_start);
	}
	else
	{
		v += m_step;
	}
	return (v.GetLongLong()<m_stop);
}
bool If::EatMe(Expression* other)
{
	If* elseIf = dynamic_cast<If*>(other);
	if (elseIf)
	{
		If* p = this;
		If* n = m_next;
		while (n != nil)
		{
			p = n;
			n = n->m_next;
		}
		p->m_next = elseIf;
		elseIf->SetParent(p);
		return true;
	}
	else
	{
		return false;
	}
}
bool If::Run(Value& v)
{
	bool bRet = true;
	bool bCanRun = false;
	if (R)
	{
		Value v0;
		bool bOK = R->Run(v0);
		if (bOK && v0 == Value(true))
		{
			bCanRun = true;
		}
	}
	else
	{//for Else in if 
		bCanRun = true;
	}
	if (bCanRun)
	{
		bRet = Block::Run(v);
	}
	else if(m_next)
	{
		bRet = m_next->Run(v);
	}
	return bRet;
}

void ColonOP::OpWithOperands(std::stack<AST::Expression*>& operands)
{
	auto operandR = operands.top();
	operands.pop();
	auto operandL = operands.top();
	operands.pop();
	auto param = new AST::Param(operandL, operandR);
	operands.push(param);
}

void CommaOp::OpWithOperands(std::stack<AST::Expression*>& operands)
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
		*list += (AST::List*)operandR;
		delete operandR;
	}
	operands.push(list);

}

}
}