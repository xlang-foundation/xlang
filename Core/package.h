#pragma once
#include "exp.h"
#include "object.h"
#include "stackframe.h"

namespace X
{
namespace AST
{
class Package :
	virtual public XPackage,
	virtual public Data::Object,
	virtual public Scope
{
	void* m_pObject = nullptr;
	StackFrame* m_stackFrame = nullptr;
public:
	Package(void* pObj):
		Data::Object(), Scope()
	{
		m_pObject = pObj;
		m_t = X::ObjType::Package;
	}
	~Package()
	{
		std::cout << "~Package()"<<std::endl;
	}
	virtual int AddMethod(const char* name) override
	{
		std::string strName(name);
		return Scope::AddOrGet(strName, false);
	}
	inline virtual AST::Scope* GetScope()
	{
		return dynamic_cast<Scope*>(this);
	}
	virtual void* GetEmbedObj() override 
	{ 
		return m_pObject; 
	}
	virtual bool Init(int varNum) override
	{
		m_stackFrame = new StackFrame(this);
		m_stackFrame->SetVarCount(varNum);
		return true;
	}
	virtual bool Call(XRuntime* rt, ARGS& params,
		KWARGS& kwParams,
		X::Value& retValue)
	{
		return true;
	}
	// Inherited via Scope
	virtual bool Set(Runtime* rt, XObj* pContext, int idx, Value& v) override
	{
		m_stackFrame->Set(idx, v);
		return true;
	}
	virtual bool SetIndexValue(XRuntime* rt, XObj* pContext, int idx, Value& v) override
	{
		return Set((Runtime*)rt, pContext, idx, v);
	}
	virtual bool Get(Runtime* rt, XObj* pContext, int idx, Value& v,
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