#pragma once
#include "exp.h"
#include "object.h"
#include "stackframe.h"

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
	virtual bool OpWithOperands(
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
		return true;
	}
	virtual void ScopeLayout() override
	{
		BinaryOp::ScopeLayout();
	}
	virtual bool Run(Runtime* rt, void* pContext,
		Value& v, LValue* lValue = nullptr) override;
};
class Package :
	public Data::Object,
	public Scope
{
	void* m_pObject = nullptr;
	StackFrame* m_stackFrame = nullptr;
public:
	Package(void* pObj):
		Data::Object(), Scope()
	{
		m_pObject = pObj;
		m_t = Data::Type::Package;
	}
	void* GetObject() { return m_pObject; }
	bool Init(int varNum)
	{
		m_stackFrame = new StackFrame(this);
		m_stackFrame->SetVarCount(varNum);
		return true;
	}
	virtual bool Call(Runtime* rt, std::vector<AST::Value>& params,
		std::unordered_map<std::string, AST::Value>& kwParams,
		AST::Value& retValue)
	{
		return true;
	}

	// Inherited via Scope
	virtual bool Set(Runtime* rt, void* pContext, int idx, Value& v) override
	{
		m_stackFrame->Set(idx, v);
		return true;
	}
	virtual bool Get(Runtime* rt, void* pContext, int idx, Value& v,
		LValue* lValue = nullptr) override
	{
		m_stackFrame->Get(idx, v, lValue);
		return true;
	}
	virtual Scope* GetParentScope() override
	{
		return nullptr;
	}
};
}
}