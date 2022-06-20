#pragma once
#include "exp.h"
#include "scope.h"
#include "block.h"

namespace X
{
namespace AST
{
enum class dbg
{
	Continue,
	Step
};

class Module :
	public Block,
	public Scope
{
	StackFrame* m_stackFrame = nullptr;
	//for debug
	dbg m_dbg = dbg::Continue;
	Expression* m_curRunningExpr = nil;
public:
	Module() :
		Scope(),
		Block()
	{
		m_stackFrame = new StackFrame(this);
		SetIndentCount({ 0,-1,-1 });//then each line will have 0 indent
	}
	~Module()
	{
		delete m_stackFrame;
	}
	virtual Scope* GetParentScope() override
	{
		return nullptr;
	}
	virtual void ScopeLayout() override;
	void AddBuiltins(Runtime* rt);
	inline void SetCurExpr(Expression* pExpr)
	{
		m_curRunningExpr = pExpr;
	}
	inline void Add(Runtime* rt, std::string& name,
		void* pContext, Value& v)
	{
		int idx = AddOrGet(name, false);
		if (idx >= 0)
		{
			//changed
			m_stackFrame->SetVarCount(GetVarNum());
			Set(rt, pContext, idx, v);
		}
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
	inline Expression* GetCurExpr()
	{
		return m_curRunningExpr;
	}
	inline void SetDbg(dbg d)
	{
		m_dbg = d;
	}
	inline dbg GetDbg() { return m_dbg; }

};
}
}