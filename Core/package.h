#pragma once
#include "exp.h"
#include "object.h"
#include "stackframe.h"

namespace X
{
namespace AST
{
	struct MemberInfo
	{
		std::string name;
		int Index = -1;
		bool KeepRawParams =false;//treat last parameter as bytes, don't do decoding
	};
class Package :
	virtual public XPackage,
	virtual public Data::Object,
	virtual public Scope
{
	void* m_pObject = nullptr;
	StackFrame* m_stackFrame = nullptr;
	std::vector<MemberInfo> m_memberInfos;
public:
	Package(void* pObj):
		Data::Object(), Scope()
	{
		m_pObject = pObj;
		m_t = X::ObjType::Package;
	}
	~Package()
	{
	}
	inline virtual int AddMethod(const char* name, bool keepRawParams=false) override
	{
		std::string strName(name);
		int idx =  Scope::AddOrGet(strName, false);
		int size = (int)m_memberInfos.size();
		if (size <= idx)
		{
			for (int i = size; i < idx; i++)
			{
				m_memberInfos.push_back({});
			}
			m_memberInfos.push_back({ name,idx,keepRawParams });
		}
		else
		{
			m_memberInfos[idx] = { name,idx,keepRawParams };
		}
		return idx;
	}
	inline virtual int QueryMethod(const char* name) override
	{
		std::string strName(name);
		return Scope::AddOrGet(strName, true);
	}
	inline virtual MemberInfo QueryMethod(std::string name)
	{
		int idx =  Scope::AddOrGet(name, true);
		return m_memberInfos[idx];
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
	virtual bool Call(XRuntime* rt, XObj* pContext,
		ARGS& params,
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
	virtual bool SetIndexValue(int idx, Value& v) override
	{
		m_stackFrame->Set(idx, v);
		return true;
	}
	virtual bool GetIndexValue(int idx, Value& v)
	{
		m_stackFrame->Get(idx,v);
		return true;
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
	virtual void GetBaseScopes(std::vector<AST::Scope*>& bases) override
	{
		bases.push_back(dynamic_cast<Scope*>(this));
	}
};
}
}