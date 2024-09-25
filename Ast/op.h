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

#pragma once
#include "exp.h"
#include "xlang.h"
#include <stack>
#include "def.h"
#include "op_registry.h"
#include "InlineCall.h"
#include "exp_exec.h"
#include "var.h"

namespace X
{
namespace AST
{
class Operator :
	public Expression
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
	virtual bool ToBytes(XlangRuntime* rt,XObj* pContext,X::XLangStream& stream) override
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
	bool GetParamList(XlangRuntime* rt, Expression* e, ARGS& params, KWARGS& kwParams);
	FORCE_INLINE void SetId(OP_ID id) { opId = id; }
	FORCE_INLINE OP_ID GetId() { return opId; }
	FORCE_INLINE short getOp(){return Op;}
	virtual void SetL(Expression* l) {}
	virtual void SetR(Expression* r) {}
	virtual bool OpWithOperands(
		std::stack<AST::Expression*>& operands,
		int LeftTokenIndex)
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
	FORCE_INLINE bool Expanding(X::Exp::ExpresionStack& stack)
	{
		stack.push({ L,false });
		stack.push({ R,false });
		return true;
	}
	virtual bool ToBytes(XlangRuntime* rt,XObj* pContext,X::XLangStream& stream) override
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
	virtual void SetScope(Scope* p) override
	{
		Expression::SetScope(p);
		if (L) L->SetScope(p);
		if (R) R->SetScope(p);
	}
	virtual bool CalcCallables(XlangRuntime* rt, XObj* pContext,
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
		std::stack<AST::Expression*>& operands, int LeftTokenIndex)
	{
		//we do a patch here, for case this.[prop1,prop2]
		//dot's Precedence is higher than '['
		//so we need to merge right pair to left side dot

		AST::Expression* operandR = nullptr;
		while (!operands.empty() && 
			operands.top()->GetTokenIndex() > m_tokenIndex)
		{
			auto r = operands.top();
			operands.pop();
			if (operandR == nullptr)
			{//must be Var
				operandR = r;
			}
			else
			{
				if (r->m_type == ObType::Dot)
				{
					auto* pDotOp = static_cast<BinaryOp*>(r);
					if (pDotOp->GetR() == nullptr)
					{
						pDotOp->SetR(operandR);
					}
					else
					{
						delete operandR;
					}
				}
				else
				{
					delete operandR;
				}
				operandR = r;
			}
		}
		if (operandR)
		{
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
	FORCE_INLINE bool ExpRun(XlangRuntime* rt, X::Exp::ExpValue& leftValue, X::Exp::ExpValue& rightValue,X::Value& retVal)
	{
		auto func = G::I().R().OpAct(Op).binaryop;
		return func ? func(rt, this, leftValue.v, rightValue.v, retVal) : false;
	}
	virtual bool Exec(XlangRuntime* rt,ExecAction& action,XObj* pContext, Value& v,LValue* lValue=nullptr) override
	{
		if (!L || !R)
		{
			return false;
		}
		Value v_l;
		if (!ExpExec(L,rt,action,pContext,v_l))
		{
			return false;
		}
		Value v_r;
		if (!ExpExec(R,rt,action,pContext, v_r))
		{
			return false;
		}
		auto func = G::I().R().OpAct(Op).binaryop;
		return func ? func(rt,this, v_l, v_r, v) : false;
	}
};
class Assign :
	public BinaryOp
{
public:
	Assign() :BinaryOp()
	{
		m_type = ObType::Assign;
	}
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
	FORCE_INLINE bool ExpRun(XlangRuntime* rt, X::Exp::ExpValue& leftValue, X::Exp::ExpValue& rightValue)
	{
		bool bOK = true;
		if (leftValue.lv)
		{
			switch (opId)
			{
			case X::OP_ID::Equ:
				*leftValue.lv = rightValue.v;
				break;
			case X::OP_ID::AddEqu:
				//TODO: need clone??
				//lValue_L->Clone();
				*leftValue.lv += rightValue.v;
				break;
			case X::OP_ID::MinusEqu:
				*leftValue.lv -= rightValue.v;
				break;
			case X::OP_ID::MulEqu:
				*leftValue.lv *= rightValue.v;
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
				//bOK = ExpSet(L, rt, pContext, v_r);
				break;
			case X::OP_ID::AddEqu:
				//v_l.Clone();
				//v_l += v_r;
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
		return bOK;
	}
	bool ObjectAssign(XlangRuntime* rt, XObj* pContext, XObj* pObj, Value& v, Value& v_r, LValue& lValue_L);
	FORCE_INLINE virtual bool Exec(XlangRuntime* rt, ExecAction& action, XObj* pContext, Value& v, LValue* lValue = nullptr) override final
	{
		if (!L || !R)
		{
			v = Value(false);
			return false;
		}
		Value v_l;
		LValue lValue_L = nullptr;
		ExpExec(L, rt, action, pContext, v_l, &lValue_L);
		Value v_r;
		if (!ExpExec(R,rt, action, pContext, v_r))
		{
			v = Value(false);
			return false;
		}
		bool bOK = true;
		if (v_l.IsObject())
		{
			auto* pObj = v_l.GetObj();
			bOK = ObjectAssign(rt, pContext, pObj, v, v_r, lValue_L);
			if (bOK)
			{
				return bOK;
			}
		}
		//move bOK to true to avoid error
		bOK = true;
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
				*lValue_L -= v_r;
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
				bOK = ExpSet(L,rt, pContext, v_r);
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
};
class ColonOP :
	public Operator
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
		std::stack<AST::Expression*>& operands, int LeftTokenIndex);
};
class CommaOp :
	public Operator
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
		std::stack<AST::Expression*>& operands, int LeftTokenIndex);
};
class SemicolonOp :
	public Operator
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
		std::stack<AST::Expression*>& operands, int LeftTokenIndex);
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
	FORCE_INLINE bool Expanding(X::Exp::ExpresionStack& stack)
	{
		stack.push({ R,false });
		return true;
	}
	virtual bool ToBytes(XlangRuntime* rt,XObj* pContext,X::XLangStream& stream) override
	{
		Operator::ToBytes(rt,pContext,stream);
		SaveToStream(rt, pContext,R, stream);
		stream << NeedParam;
		return true;
	}
	virtual void SetScope(Scope* p) override
	{
		Expression::SetScope(p);
		if (R) R->SetScope(p);
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
		std::stack<AST::Expression*>& operands, int LeftTokenIndex)
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
	FORCE_INLINE Expression* GetR() { return R; }

	virtual bool Exec(XlangRuntime* rt,ExecAction& action,XObj* pContext, Value& v,LValue* lValue=nullptr) override;
};

//will be used in if x in container or while x in container 
//but for x in container doesn't need to be handled by this InOp
//for will handle it by itself
class InOp :
	public BinaryOp
{

public:
	InOp() :
		BinaryOp()
	{
		m_type = ObType::In;
	}
	InOp(short op) :
		BinaryOp(op)
	{
		m_type = ObType::In;
	}
	virtual bool Exec(XlangRuntime* rt, ExecAction& action, XObj* pContext, Value& v, LValue* lValue = nullptr) override final;
};
class ExternDecl :
	public UnaryOp
{
public:
	ExternDecl() :
		UnaryOp()
	{
		m_type = ObType::ExternDecl;
	}
	ExternDecl(short op) :
		UnaryOp(op)
	{
		m_type = ObType::ExternDecl;
	}
	virtual void ScopeLayout() override;
	FORCE_INLINE virtual bool Exec(XlangRuntime* rt,ExecAction& action, XObj* pContext, Value& v, LValue* lValue = nullptr) override
	{
		return true;//dont' run Base class's Run
	}
};
}
}