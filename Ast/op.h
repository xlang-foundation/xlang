#pragma once
#include "exp.h"
#include "xlang.h"
#include <stack>
#include "def.h"
#include "op_registry.h"

namespace X
{
namespace AST
{
class Operator :
	virtual public Expression
{
protected:
	short Op=0;//index of _kws
	OP_ID opId = OP_ID::None;
public:
	Operator()
	{
	}
	Operator(short op)
	{
		Op = op;
	}
	virtual bool ToBytes(Runtime* rt,XObj* pContext,X::XLangStream& stream) override
	{
		Expression::ToBytes(rt,pContext,stream);
		stream << Op;
		stream << opId;
		return true;
	}
	virtual bool FromBytes(X::XLangStream& stream) override
	{
		Expression::FromBytes(stream);
		stream >> Op;
		stream >> opId;
		return true;
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
	virtual public Operator
{
protected:
	Expression* L=nil;
	Expression* R = nil;
public:
	BinaryOp()
	{
		m_type = ObType::BinaryOp;
	}
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
	virtual bool ToBytes(Runtime* rt,XObj* pContext,X::XLangStream& stream) override
	{
		Operator::ToBytes(rt,pContext,stream);
		SaveToStream(rt, pContext,L, stream);
		SaveToStream(rt, pContext,R, stream);
		return true;
	}
	virtual bool FromBytes(X::XLangStream& stream) override
	{
		Operator::FromBytes(stream);
		L = BuildFromStream<Expression>(stream);
		R = BuildFromStream<Expression>(stream);
		return true;
	}

	virtual bool CalcCallables(Runtime* rt, XObj* pContext,
		std::vector<Scope*>& callables) override
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
			ReCalcHint(L);
			L->SetParent(this);
		}
	}
	virtual void SetR(Expression* r) override
	{
		R = r;
		if (R)
		{
			ReCalcHint(R);
			R->SetParent(this);
		}
	}
	Expression* GetR() { return R; }
	Expression* GetL() { return L; }

	virtual bool Run(Runtime* rt,XObj* pContext, Value& v,LValue* lValue=nullptr) override
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
		auto func = G::I().R().OpAct(Op).binaryop;
		return func ? func(rt,this, v_l, v_r, v) : false;
	}
};
class Assign :
	virtual public BinaryOp
{
public:
	Assign() :Operator(), BinaryOp()
	{
		m_type = ObType::Assign;
	}
	Assign(short op) :
		Operator(op),
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
	virtual bool Run(Runtime* rt, XObj* pContext, Value& v, LValue* lValue = nullptr) override;
};
class ColonOP :
	virtual public Operator
{
public:
	ColonOP() :Operator()
	{
		m_type = ObType::ColonOp;
	}
	ColonOP(short op) :
		Operator(op)
	{
		m_type = ObType::ColonOp;
	}
	virtual bool OpWithOperands(
		std::stack<AST::Expression*>& operands);
};
class CommaOp :
	virtual public Operator
{
public:
	CommaOp() :Operator()
	{
		m_type = ObType::CommaOp;
	}
	CommaOp(short op) :
		Operator(op)
	{
		m_type = ObType::CommaOp;
	}
	virtual bool OpWithOperands(
		std::stack<AST::Expression*>& operands);
};
class SemicolonOp :
	virtual public Operator
{
public:
	SemicolonOp() :Operator()
	{
		m_type = ObType::SemicolonOp;
	}
	SemicolonOp(short op) :
		Operator(op)
	{
		m_type = ObType::SemicolonOp;
	}
	virtual bool OpWithOperands(
		std::stack<AST::Expression*>& operands);
};

class UnaryOp :
	virtual public Operator
{
protected:
	Expression* R = nil;
	bool NeedParam = true;//like else(), it is false
public:
	UnaryOp()
	{
		m_type = ObType::UnaryOp;
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
	virtual bool ToBytes(Runtime* rt,XObj* pContext,X::XLangStream& stream) override
	{
		Operator::ToBytes(rt,pContext,stream);
		SaveToStream(rt, pContext,R, stream);
		stream << NeedParam;
		return true;
	}
	virtual bool FromBytes(X::XLangStream& stream) override
	{
		Operator::FromBytes(stream);
		R = BuildFromStream<Expression>(stream);
		stream >> NeedParam;
		return true;
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
		if (NeedParam && operands.size()>0)
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
			ReCalcHint(R);
			R->SetParent(this);
		}
	}
	Expression* GetR() { return R; }

	virtual bool Run(Runtime* rt,XObj* pContext, Value& v,LValue* lValue=nullptr) override;
};
class Range :
	virtual public UnaryOp
{
	bool m_evaluated = false;
	long long m_start=0;
	long long m_stop =0;
	long long m_step = 1;

	bool Eval(Runtime* rt);
public:
	Range() :
		Operator(),
		UnaryOp()
	{
		m_type = ObType::Range;
	}
	Range(short op) :
		Operator(op),
		UnaryOp(op)
	{
		m_type = ObType::Range;
	}
	virtual bool ToBytes(Runtime* rt,XObj* pContext,X::XLangStream& stream) override
	{
		UnaryOp::ToBytes(rt,pContext,stream);
		SaveToStream(rt, pContext,R, stream);
		stream << m_evaluated << m_start<< m_stop<< m_step;
		return true;
	}
	virtual bool FromBytes(X::XLangStream& stream) override
	{
		UnaryOp::FromBytes(stream);
		R = BuildFromStream<Expression>(stream);
		stream >> m_evaluated >> m_start >> m_stop >> m_step;
		return true;
	}
	virtual bool Run(Runtime* rt,XObj* pContext, Value& v,LValue* lValue=nullptr) override;
};
class InOp :
	virtual public BinaryOp
{
public:
	InOp() :
		Operator(),
		BinaryOp()
	{
		m_type = ObType::In;
	}
	InOp(short op) :
		Operator(op),
		BinaryOp(op)
	{
		m_type = ObType::In;
	}
	inline virtual bool Run(Runtime* rt,XObj* pContext, Value& v,LValue* lValue=nullptr) override
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
class ExternDecl :
	virtual public UnaryOp
{
public:
	ExternDecl() :
		Operator(),
		UnaryOp()
	{
		m_type = ObType::ExternDecl;
	}
	ExternDecl(short op) :
		Operator(op),
		UnaryOp(op)
	{
		m_type = ObType::ExternDecl;
	}
	virtual void ScopeLayout() override;
	inline virtual bool Run(Runtime* rt, XObj* pContext, Value& v, LValue* lValue = nullptr) override
	{
		return true;//dont' run Base class's Run
	}
};
}
}