#pragma once
#include "exp.h"
#include "object.h"
#include "stackframe.h"
#include <functional>

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
	PackageCleanup m_funcPackageCleanup = nullptr;
	virtual void SetPackageCleanupFunc(PackageCleanup func) override
	{
		m_funcPackageCleanup = func;
	}
public:
	Package(void* pObj):
		Data::Object(), Scope()
	{
		m_pObject = pObj;
		m_t = X::ObjType::Package;
	}
	~Package()
	{
		if (m_pObject)
		{
			Cleanup(m_pObject);
		}
	}
	inline void Cleanup(void* pObj)
	{
		if (m_funcPackageCleanup)
		{
			m_funcPackageCleanup(pObj);
		}
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
	inline virtual int QueryMethod(const char* name, bool* pKeepRawParams = nullptr) override
	{
		std::string strName(name);
		int idx =  Scope::AddOrGet(strName, true);
		if (pKeepRawParams)
		{
			*pKeepRawParams = m_memberInfos[idx].KeepRawParams;
		}
		return idx;
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
class PackageProxy :
	virtual public XPackage,
	virtual public Data::Object,
	virtual public Scope
{
	void* m_pObject = nullptr;
	Package* m_pPackage = nullptr;
	PackageCleanup m_funcPackageCleanup = nullptr;
	virtual void SetPackageCleanupFunc(PackageCleanup func) override
	{
		m_funcPackageCleanup = func;
	}
public:
	PackageProxy(Package* pPack,void* pObj) :
		Data::Object(), Scope()
	{
		m_pPackage = pPack;
		if (m_pPackage)
		{
			m_pPackage->Scope::IncRef();
		}
		m_pObject = pObj;
		m_t = X::ObjType::Package;
	}
	~PackageProxy()
	{
		if (m_funcPackageCleanup)
		{
			m_funcPackageCleanup(m_pObject);
		}
		else
		{
			if (m_pPackage)
			{
				m_pPackage->Cleanup(m_pObject);
			}
		}
		if (m_pPackage)
		{
			m_pPackage->Scope::DecRef();
		}
	}
	inline virtual int AddMethod(const char* name, bool keepRawParams = false) override
	{
		return m_pPackage->AddMethod(name, keepRawParams);
	}
	inline virtual int QueryMethod(const char* name, bool* pKeepRawParams = nullptr) override
	{
		return m_pPackage->QueryMethod(name, pKeepRawParams);
	}
	inline virtual MemberInfo QueryMethod(std::string name)
	{
		return m_pPackage->QueryMethod(name);
	}
	inline virtual AST::Scope* GetScope()
	{
		return m_pPackage->GetScope();
	}
	virtual void* GetEmbedObj() override
	{
		return m_pObject;
	}
	// Inherited via Scope
	inline virtual bool Set(Runtime* rt, XObj* pContext, int idx, Value& v) override
	{
		return m_pPackage->Set(rt,pContext,idx,v);
	}
	inline virtual bool SetIndexValue(int idx, Value& v) override
	{
		return m_pPackage->SetIndexValue(idx,v);
	}
	inline virtual bool GetIndexValue(int idx, Value& v)
	{
		return m_pPackage->GetIndexValue(idx,v);
	}
	inline virtual bool Get(Runtime* rt, XObj* pContext, int idx, Value& v,
		LValue* lValue = nullptr) override
	{
		return m_pPackage->Get(rt,pContext,idx,v,lValue);
	}
	inline virtual Scope* GetParentScope() override
	{
		return m_pPackage->GetParentScope();
	}
	inline virtual void GetBaseScopes(std::vector<AST::Scope*>& bases) override
	{
		m_pPackage->GetBaseScopes(bases);
	}
	virtual bool Init(int varNum) override
	{
		return true;
	}
};
}
}