#pragma once
#include "exp.h"
#include "scope.h"
#include "func.h"
#include "stackframe.h"

namespace X
{
namespace AST
{
#define FastMatchThis(name) (name.size() ==4 \
	&& name[0] =='t' && name[0] =='h' && name[0] =='i' && name[0] =='s')
class XClass
	:public Func
{
	Func* m_constructor = nil;
	AST::StackFrame* m_variableFrame = nullptr;//to hold non-instance properties
	std::vector<Value> m_bases;
	XClass* FindBase(XlangRuntime* rt, std::string& strName);
public:
	XClass() :
		Func()
	{
		m_type = ObType::Class;
		m_variableFrame = new StackFrame(m_pMyScope);
		m_pMyScope->SetVarFrame(m_variableFrame);
	}
	~XClass()
	{
		if (m_variableFrame)
		{
			delete m_variableFrame;
		}
	}
	FORCE_INLINE int QueryConstructor()
	{
		//Constructor can be class Name as its function name
		//Or constructor uses name: constructor or __init__
		auto name = GetNameString();
		SCOPE_FAST_CALL_AddOrGet0(idx,m_pMyScope,name,true);
		if (idx>=0)
		{
			return idx;
		}
		name = "constructor";
		SCOPE_FAST_CALL_AddOrGet0_NoDef(idx,m_pMyScope,name, true);
		if (idx >= 0)
		{
			return idx;
		}
		name = "__init__";
		SCOPE_FAST_CALL_AddOrGet0_NoDef(idx,m_pMyScope,name, true);
		if (idx >= 0)
		{
			return idx;
		}
		return -1;
	}
	FORCE_INLINE StackFrame* GetClassStack()
	{
		return m_variableFrame;
	}
	virtual bool ToBytes(XlangRuntime* rt, XObj* pContext, X::XLangStream& stream) override
	{
		std::string code;
		for (auto* decor : m_decors)
		{
			code += decor->GetCode() + "\n";
		}
		code += GetCode();
		//change current scope of stream
		Scope* pOldClassScope = stream.ScopeSpace().GetCurrentClassScope();
		Scope* pOldScope = stream.ScopeSpace().GetCurrentScope();
		auto* pCurScope = dynamic_cast<Scope*>(this);
		stream.ScopeSpace().SetCurrentScope(pCurScope);
		stream.ScopeSpace().SetCurrentClassScope(pCurScope);
		Block::ToBytes(rt, pContext, stream);
		SaveToStream(rt, pContext, Params, stream);
		SaveToStream(rt, pContext, RetType, stream);
		//restore old scope
		stream.ScopeSpace().SetCurrentScope(pOldScope);
		stream.ScopeSpace().SetCurrentClassScope(pOldClassScope);

		//Coding itself
		stream << m_Name.size;
		if (m_Name.size > 0)
		{
			stream.append(m_Name.s, m_Name.size);
		}
		stream << (int)m_IndexofParamList.size();
		for (auto idx : m_IndexofParamList)
		{
			stream << idx;
		}
		stream << m_Index << m_IndexOfThis << m_needSetHint;
		m_pMyScope->ToBytes(rt, pContext, stream);

		return true;
	}
	virtual bool FromBytes(X::XLangStream& stream) override;
	int AddOrGet(std::string& name, bool bGetOnly, Scope** ppRightScope = nullptr);
	int AddAndSet(XlangRuntime* rt, XObj* pContext, std::string& name, Value& v)
	{
		SCOPE_FAST_CALL_AddOrGet0(idx,m_pMyScope,name, false);
		if (idx >= 0)
		{
			int cnt = m_variableFrame->GetVarCount();
			if (cnt <= idx)
			{
				m_variableFrame->SetVarCount(idx + 1);
			}
			m_variableFrame->Set(idx, v);
		}
		return idx;
	}
	bool Set(XlangRuntime* rt, XObj* pContext, int idx, Value& v);
	bool Get(XlangRuntime* rt, XObj* pContext, int idx, Value& v,LValue* lValue = nullptr);
	FORCE_INLINE std::vector<Value>& GetBases() { return m_bases; }
	bool Exec_i(XlangRuntime* rt, ExecAction& action, XObj* pContext, Value& v, LValue* lValue = nullptr);
	bool BuildBaseInstances(XlangRuntime* rt, XObj* pClassObj);
	virtual bool Exec(XlangRuntime* rt,ExecAction& action, XObj* pContext, Value& v, LValue* lValue = nullptr) override;
	virtual void ScopeLayout() override;
	virtual void Add(Expression* item) override;
	virtual bool Call(XlangRuntime* rt,
		XObj* pContext,
		ARGS& params,
		KWARGS& kwParams,
		Value& retValue);
	virtual bool CalcCallables(XlangRuntime* rt, XObj* pContext,
		std::vector<Scope*>& callables) override
	{
		bool bHave = false;
		if (m_constructor)
		{
			bHave = m_constructor->CalcCallables(rt, pContext, callables);
		}
		return bHave;
	}
};
}
}