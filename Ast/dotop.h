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
#include "op.h"
#include "number.h"
#include "var.h"

namespace X
{
	namespace Data { class Object; }
namespace AST
{
class DotOp :
	public BinaryOp
{
	int m_dotNum = 1;
protected:
	void QueryBases(XlangRuntime* rt, Data::Object* pObj,
		std::vector<Scope*>& bases);
	void RunScopeLayoutWithScopes(Expression* pExpr, 
		std::vector<Scope*>& scopes);
	bool DotProcess(XlangRuntime* rt, XObj* pContext, 
		Value& v_l, Expression* r,
		Value& v, LValue* lValue = nullptr);
public:
	DotOp() :BinaryOp()
	{
		m_type = ObType::Dot;
	}
	DotOp(short opIndex) :
		BinaryOp(opIndex)
	{
		m_type = ObType::Dot;
		m_dotNum = 1;
	}
	DotOp(short opIndex,int dotNum) :
		BinaryOp(opIndex)
	{
		m_type = ObType::Dot;
		m_dotNum = dotNum;
	}
	virtual bool ToBytes(XlangRuntime* rt,XObj* pContext,X::XLangStream& stream) override
	{
		BinaryOp::ToBytes(rt,pContext,stream);
		stream << m_dotNum;
		return true;
	}
	virtual bool FromBytes(X::XLangStream& stream) override
	{
		BinaryOp::FromBytes(stream);
		stream >> m_dotNum;
		return true;
	}
	virtual bool Exec(XlangRuntime* rt,ExecAction& action,XObj* pContext, Value& v, LValue* lValue = nullptr) override;
	virtual void ScopeLayout() override;
	virtual bool CalcCallables(XlangRuntime* rt, XObj* pContext,
		std::vector<Scope*>& callables) override;

	FORCE_INLINE bool Expanding(X::Exp::ExpresionStack& stack)
	{
		//we keep R not in stack to cascade dot ops working
		//such as obj1.obj2.obj3
		stack.push({ L,false });
		return true;
	}

	FORCE_INLINE bool ExpRun(XlangRuntime* rt, XObj* pContext,
		X::Exp::ExpValue& leftValue, 
		X::Value& retVal, LValue* lValue)
	{
		auto& v_l = leftValue.v;
		if (v_l.IsNumber())
		{
			double dValue = (double)v_l;
			double fraction = 0;
			int digiNum = 0;

			if (R->m_type == ObType::Number)
			{
				Number* pNum = dynamic_cast<Number*>(R);
				fraction = (double)pNum->GetVal();
				digiNum = pNum->GetDigiNum();
			}
			else if (R->m_type == ObType::Var)
			{
				String name = (static_cast<Var*>(R))->GetName();
				double dVal = 0;
				long long llVal = 0;
				ParseState st = ParseNumber(name, dVal, llVal);
				if (st == ParseState::Long_Long)
				{
					digiNum = (int)dVal;
					fraction = (double)llVal;
				}
			}
			else
			{//TODO: error
				return false;
			}
			//TODO:optimize here
			for (int i = 0; i < digiNum; i++)
			{
				fraction /= 10;
			}
			dValue += fraction;
			retVal = Value(dValue);
			return true;

		}
		Expression* r = R;
		while (r->m_type == ObType::Dot)
		{
			DotOp* dotR = dynamic_cast<DotOp*>(r);
			Value v0;
			LValue lValue0 = nil;
			DotProcess(rt, pContext, v_l, dotR->GetL(), v0, &lValue0);
			v_l = v0;
			r = dotR->GetR();
		}
		DotProcess(rt, pContext, v_l, r, retVal, lValue);
		return true;
	}
};
}
}