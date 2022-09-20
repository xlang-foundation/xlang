#pragma once
#include "exp.h"
#include "op.h"

namespace X
{
	namespace Data { class Object; }
namespace AST
{
class DotOp :
	virtual public BinaryOp
{
	int m_dotNum = 1;
protected:
	void QueryBases(Runtime* rt, Data::Object* pObj,
		std::vector<Scope*>& bases);
	void RunScopeLayoutWithScopes(Expression* pExpr, 
		std::vector<Scope*>& scopes);
	bool DotProcess(Runtime* rt, XObj* pContext, 
		Value& v_l, Expression* r,
		Value& v, LValue* lValue = nullptr);
public:
	DotOp() :Operator(), BinaryOp()
	{
		m_type = ObType::Dot;
	}
	DotOp(short opIndex) :
		Operator(opIndex),
		BinaryOp(opIndex)
	{
		m_type = ObType::Dot;
		m_dotNum = 1;
	}
	DotOp(short opIndex,int dotNum) :
		Operator(opIndex),
		BinaryOp(opIndex)
	{
		m_type = ObType::Dot;
		m_dotNum = dotNum;
	}
	virtual bool ToBytes(Runtime* rt,XObj* pContext,X::XLangStream& stream) override
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
	virtual bool Run(Runtime* rt,XObj* pContext, Value& v, LValue* lValue = nullptr) override;
	virtual void ScopeLayout() override;
	virtual bool CalcCallables(Runtime* rt, XObj* pContext,
		std::vector<Scope*>& callables) override;
};
}
}