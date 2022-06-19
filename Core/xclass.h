#pragma once
#include "exp.h"
#include "scope.h"
#include "func.h"

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
	AST::StackFrame* m_stackFrame = nullptr;//to hold non-instance properties
	std::vector<XClass*> m_bases;
	std::vector<std::pair<int, Value>> m_tempMemberList;
	XClass* FindBase(Runtime* rt, std::string& strName);
public:
	XClass() :
		Func()
	{
		m_type = ObType::Class;
	}
	inline StackFrame* GetClassStack()
	{
		return m_stackFrame;
	}

	virtual bool Set(Runtime* rt, void* pContext, int idx, Value& v) override;
	virtual bool Get(Runtime* rt, void* pContext, int idx, Value& v,
		LValue* lValue = nullptr) override;
	inline std::vector<XClass*>& GetBases() { return m_bases; }
	virtual bool Run(Runtime* rt, void* pContext, Value& v, LValue* lValue = nullptr) override;
	virtual void ScopeLayout() override;
	virtual void Add(Expression* item) override;
	virtual bool Call(Runtime* rt,
		void* pContext,
		std::vector<Value>& params,
		std::unordered_map<std::string, AST::Value>& kwParams,
		Value& retValue);
};
}
}