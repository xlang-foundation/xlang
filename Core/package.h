#pragma once
#include "exp.h"
#include "object.h"

namespace X
{
namespace AST
{
class Import :
	public BinaryOp
{
public:
	Import(short op) :
		BinaryOp(op)
	{
		m_type = ObType::Import;
	}
	virtual void OpWithOperands(
		std::stack<AST::Expression*>& operands)
	{
		auto operandR = operands.top();
		operands.pop();
		SetR(operandR);
		if (!operands.empty())
		{//from can be igored
			auto operandL = operands.top();
			operands.pop();
			SetL(operandL);
		}
		operands.push(this);
	}
	virtual void ScopeLayout() override
	{
		BinaryOp::ScopeLayout();
	}
	virtual bool Run(Runtime* rt, void* pContext,
		Value& v, LValue* lValue = nullptr) override;
};
class Package :
	public Data::Object
{
public:
	virtual bool Call(Runtime* rt, std::vector<AST::Value>& params,
		std::unordered_map<std::string, AST::Value>& kwParams,
		AST::Value& retValue)
	{
		return true;
	}
};
}
}