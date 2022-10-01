#pragma once
#include "exp.h"
#include "scope.h"
#include "func.h"

#define EVENT_OBJ_TYPE_NAME "event"
namespace X
{
namespace AST
{
#define FastMatchThis(name) (name.size() ==4 \
	&& name[0] =='t' && name[0] =='h' && name[0] =='i' && name[0] =='s')
class XClass
	:virtual public Func
{
	struct MemberInfo
	{
		int index;
		std::string typeName;
		Value defaultValue;
	};
	Func* m_constructor = nil;
	AST::StackFrame* m_stackFrame = nullptr;//to hold non-instance properties
	std::vector<XClass*> m_bases;
	std::vector<MemberInfo> m_tempMemberList;
	XClass* FindBase(Runtime* rt, std::string& strName);
public:
	XClass() :
		Func()
	{
		m_type = ObType::Class;
	}
	~XClass()
	{
		if (m_stackFrame)
		{
			delete m_stackFrame;
		}
	}
	inline StackFrame* GetClassStack()
	{
		return m_stackFrame;
	}
	virtual void AddAndSet(Runtime* rt, XObj* pContext, std::string& name, Value& v) override
	{
		int idx = AddOrGet(name, false);
		if (idx >= 0)
		{
			int cnt = m_stackFrame->GetVarCount();
			if (cnt <= idx)
			{
				m_stackFrame->SetVarCount(idx + 1);
			}
			Set(rt, pContext, idx, v);
		}
	}
	virtual bool Set(Runtime* rt, XObj* pContext, int idx, Value& v) override;
	virtual bool Get(Runtime* rt, XObj* pContext, int idx, Value& v,
		LValue* lValue = nullptr) override;
	inline std::vector<XClass*>& GetBases() { return m_bases; }
	virtual bool Run(Runtime* rt, XObj* pContext, Value& v, LValue* lValue = nullptr) override;
	virtual void ScopeLayout() override;
	virtual void Add(Expression* item) override;
	virtual bool Call(Runtime* rt,
		XObj* pContext,
		std::vector<Value>& params,
		KWARGS& kwParams,
		Value& retValue);
	virtual bool CalcCallables(Runtime* rt, XObj* pContext,
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