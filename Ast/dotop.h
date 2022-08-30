#pragma once
#include "exp.h"
#include "op.h"

namespace X
{
namespace AST
{
class DotOp :
	virtual public BinaryOp
{
	int m_dotNum = 1;
protected:
	void QueryBases(Runtime* rt,void* pObj0,
		std::vector<Scope*>& bases);
	void RunScopeLayoutWithScopes(Expression* pExpr, 
		std::vector<Scope*>& scopes);
	bool DotProcess(Runtime* rt, void* pContext, 
		Value& v_l, Expression* r,
		Value& v, LValue* lValue = nullptr);
public:
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
	virtual bool Run(Runtime* rt,void* pContext, Value& v, LValue* lValue = nullptr) override;
	virtual void ScopeLayout() override;
	virtual bool CalcCallables(Runtime* rt, void* pContext,
		std::vector<Scope*>& callables) override;
};
}
}