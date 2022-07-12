#pragma once
#include "exp.h"
#include <stack>

namespace X
{
namespace AST
{
class Operator :
	public Expression
{
protected:
	short Op;//index of _kws
	OP_ID opId = OP_ID::None;
public:
	Operator()
	{
		Op = 0;
	}
	Operator(short op)
	{
		Op = op;
	}
	bool GetParamList(Runtime* rt, Expression* e, ARGS& params, KWARGS& kwParams);
	inline void SetId(OP_ID id) { opId = id; }
	inline OP_ID GetId() { return opId; }
	inline short getOp(){return Op;}
	virtual void SetL(Expression* l) {}
	virtual void SetR(Expression* r) {}
	virtual bool OpWithOperands(
		std::stack<AST::Expression*>& operands) 
	{
		return true;//OP finished
	}
};

class BinaryOp :
	public Operator
{
protected:
	Expression* L=nil;
	Expression* R = nil;
public:
	BinaryOp(short op):
		Operator(op)
	{
		m_type = ObType::BinaryOp;
	}
	~BinaryOp()
	{
		if (L) delete L;
		if (R) delete R;
	}
	virtual bool CalcCallables(Runtime* rt, void* pContext,
		std::vector<Expression*>& callables) override
	{
		bool bHave = false;
		if (L)
		{
			bHave = L->CalcCallables(rt, pContext,callables);
		}
		if (R)
		{
			bHave |= R->CalcCallables(rt, pContext,callables);
		}
		return bHave;
	}
	virtual int GetLeftMostCharPos() override
	{
		int pos = GetCharPos();
		int startLine = GetStartLine();
		if (L)
		{
			int posL = L->GetLeftMostCharPos();
			if (posL < pos && L->GetStartLine() <= startLine)
			{
				pos = posL;
			}
		}
		if (R)
		{
			int posR = R->GetLeftMostCharPos();
			if (posR < pos && R->GetStartLine() <= startLine)
			{
				pos = posR;
			}
		}
		return pos;
	}
	virtual bool OpWithOperands(
		std::stack<AST::Expression*>& operands)
	{
		if (!operands.empty())
		{
			auto operandR = operands.top();
			operands.pop();
			SetR(operandR);
		}
		if (!operands.empty())
		{
			auto operandL = operands.top();
			operands.pop();
			SetL(operandL);
		}
		operands.push(this);
		return true;
	}
	virtual void ScopeLayout() override
	{
		if (L) L->ScopeLayout();
		if (R) R->ScopeLayout();
	}
	virtual void SetL(Expression* l) override
	{
		L = l;
		if (L) 
		{
			L->SetParent(this);
		}
	}
	virtual void SetR(Expression* r) override
	{
		R = r;
		if (R)
		{
			R->SetParent(this);
		}
	}
	Expression* GetR() { return R; }
	Expression* GetL() { return L; }

	virtual bool Run(Runtime* rt,void* pContext, Value& v,LValue* lValue=nullptr) override
	{
		if (!L || !R)
		{
			return false;
		}
		Value v_l;
		if (!L->Run(rt,pContext,v_l))
		{
			return false;
		}
		Value v_r;
		if (!R->Run(rt,pContext, v_r))
		{
			return false;
		}
		auto func = G::I().OpAct(Op).binaryop;
		return func ? func(rt,this, v_l, v_r, v) : false;
	}
};
class Assign :
	public BinaryOp
{
	bool AssignToDataObject(Runtime* rt,void* pObjPtr);
public:
	Assign(short op) :
		BinaryOp(op)
	{
		m_type = ObType::Assign;
	}
	virtual void SetL(Expression* l) override
	{
		BinaryOp::SetL(l);
		if (L)
		{
			L->SetIsLeftValue(true);
		}
	}
	virtual bool Run(Runtime* rt,void* pContext, Value& v,LValue* lValue=nullptr) override
	{
		if (!L || !R)
		{
			return false;
		}
		Value v_l;
		LValue lValue_L = nullptr;
		L->Run(rt,pContext, v_l, &lValue_L);
		if (v_l.IsObject())
		{
			bool bOK = AssignToDataObject(rt,v_l.GetObj());
			if (bOK)
			{
				return true;
			}
		}
		Value v_r;
		if (R->Run(rt,pContext, v_r))
		{
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
		}
		return true;
	}
};
class ColonOP :
	public Operator
{
public:
	ColonOP(short op) :
		Operator(op)
	{
	}
	virtual bool OpWithOperands(
		std::stack<AST::Expression*>& operands);
};
class CommaOp :
	public Operator
{
public:
	CommaOp(short op) :
		Operator(op)
	{
	}
	virtual bool OpWithOperands(
		std::stack<AST::Expression*>& operands);
};
class SemicolonOp :
	public Operator
{
public:
	SemicolonOp(short op) :
		Operator(op)
	{
	}
	virtual bool OpWithOperands(
		std::stack<AST::Expression*>& operands);
};

class UnaryOp :
	public Operator
{
protected:
	Expression* R = nil;
	bool NeedParam = true;//like else(), it is false
public:
	UnaryOp()
	{
	}
	UnaryOp(short op):
		Operator(op)
	{
		m_type = ObType::UnaryOp;
	}
	~UnaryOp()
	{
		if (R) delete R;
	}
	virtual int GetLeftMostCharPos() override
	{
		int pos = GetCharPos();
		int startLine = GetStartLine();
		if (R)
		{
			int posR = R->GetLeftMostCharPos();
			if (posR < pos && R->GetStartLine() <= startLine)
			{
				pos = posR;
			}
		}
		return pos;
	}
	virtual void ScopeLayout() override
	{
		if (R) R->ScopeLayout();
	}
	virtual bool OpWithOperands(
		std::stack<AST::Expression*>& operands)
	{
		if (NeedParam)
		{
			auto operandR = operands.top();
			operands.pop();
			SetR(operandR);
		}
		operands.push(this);
		return true;
	}
	virtual void SetR(Expression* r) override
	{
		R = r;
		if (R)
		{
			R->SetParent(this);
		}
	}
	Expression* GetR() { return R; }

	virtual bool Run(Runtime* rt,void* pContext, Value& v,LValue* lValue=nullptr) override;
};
class Range :
	public UnaryOp
{
	bool m_evaluated = false;
	long long m_start=0;
	long long m_stop =0;
	long long m_step = 1;

	bool Eval(Runtime* rt);
public:
	Range(short op) :
		UnaryOp(op)
	{
		m_type = ObType::Range;
	}

	virtual bool Run(Runtime* rt,void* pContext, Value& v,LValue* lValue=nullptr) override;
};
class InOp :
	public BinaryOp
{
public:
	InOp(short op) :
		BinaryOp(op)
	{
	}
	inline virtual bool Run(Runtime* rt,void* pContext, Value& v,LValue* lValue=nullptr) override
	{
		bool bIn = R->Run(rt,pContext, v);
		if(bIn)
		{
			L->Set(rt,pContext, v);
		}
		return bIn;
	}
	virtual void SetL(Expression* l) override
	{
		BinaryOp::SetL(l);
		if (l)
		{
			l->SetIsLeftValue(true);
		}
	}
};

}
}