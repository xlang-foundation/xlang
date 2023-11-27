#pragma once
#include "exp.h"
#include "op.h"

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
};
}
}