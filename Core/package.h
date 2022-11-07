#pragma once
#include "exp.h"
#include "object.h"
#include "stackframe.h"
#include <functional>

namespace X
{
	class APISetBase;
namespace AST
{
	struct MemberIndexInfo
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
	APISetBase* m_pAPISet = nullptr;
	void* m_pObject = nullptr;
	StackFrame* m_stackFrame = nullptr;
	std::vector<MemberIndexInfo> m_memberInfos;
	PackageCleanup m_funcPackageCleanup = nullptr;
	virtual void SetPackageCleanupFunc(PackageCleanup func) override
	{
		m_funcPackageCleanup = func;
	}
public:
	APISetBase* GetAPISet() { return m_pAPISet; }
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
	void SetAPISet(APISetBase* p)
	{
		m_pAPISet = p;
	}
	virtual X::Data::List* FlatPack(XlangRuntime* rt, XObj* pContext,
		std::vector<std::string>& IdList, int id_offset,
		long long startIndex, long long count) override;
	virtual X::Value UpdateItemValue(XlangRuntime* rt, XObj* pContext,
		std::vector<std::string>& IdList, int id_offset,
		std::string itemName, X::Value& val) override;
	virtual long long Size()
	{
		return (long long)m_memberInfos.size();
	}
	virtual void RemoveALl() override
	{
		if (m_stackFrame)
		{
			delete m_stackFrame;
			m_stackFrame = nullptr;
		}
	}
	StackFrame* GetStack() { return m_stackFrame; }
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
		if (idx>=0 && pKeepRawParams)
		{
			*pKeepRawParams = m_memberInfos[idx].KeepRawParams;
		}
		return idx;
	}
	inline virtual MemberIndexInfo QueryMethod(std::string name)
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
	virtual bool Set(XlangRuntime* rt, XObj* pContext, int idx, Value& v) override
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
	virtual bool Get(XlangRuntime* rt, XObj* pContext, int idx, Value& v,
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
	//for functions, just addref
	//for event, need to clone a new event
	//for prop, TODO:
	//all add into m_stackFrame
	StackFrame* m_stackFrame = nullptr;
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
			m_stackFrame = new StackFrame(this);
			auto* pBaseStack = m_pPackage->GetStack();
			int cnt = pBaseStack->GetVarCount();
			m_stackFrame->SetVarCount(cnt);
			for (int i = 0; i < cnt; i++)
			{
				X::Value v0;
				pBaseStack->Get(i, v0);
				if (v0.IsObject() && v0.GetObj()->GetType() == ObjType::ObjectEvent)
				{
					v0.Clone();
				}
				m_stackFrame->Set(i, v0);
			}
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
		if (m_stackFrame)
		{
			delete m_stackFrame;
			m_stackFrame = nullptr;
		}
	}
	virtual X::Data::List* FlatPack(XlangRuntime* rt, XObj* pContext,
		std::vector<std::string>& IdList, int id_offset,
		long long startIndex, long long count) override;
	virtual X::Value UpdateItemValue(XlangRuntime* rt, XObj* pContext,
		std::vector<std::string>& IdList, int id_offset,
		std::string itemName, X::Value& val) override;
	virtual long long Size()
	{
		return m_pPackage->Size();
	}
	virtual int AddOrGet(std::string& name, bool bGetOnly)
	{
		return m_pPackage->AddOrGet(name, bGetOnly);
	}
	virtual void RemoveALl() override
	{
		m_pPackage->RemoveALl();
		if (m_stackFrame)
		{
			delete m_stackFrame;
			m_stackFrame = nullptr;
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
	inline virtual MemberIndexInfo QueryMethod(std::string name)
	{
		return m_pPackage->QueryMethod(name);
	}
	inline virtual AST::Scope* GetScope()
	{
		return this;
	}
	virtual void* GetEmbedObj() override
	{
		return m_pObject;
	}
	// Inherited via Scope
	virtual bool Set(XlangRuntime* rt, XObj* pContext, int idx, Value& v) override
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
		m_stackFrame->Get(idx, v);
		return true;
	}
	virtual bool Get(XlangRuntime* rt, XObj* pContext, int idx, Value& v,
		LValue* lValue = nullptr) override
	{
		m_stackFrame->Get(idx, v, lValue);
		return true;
	}
	inline virtual Scope* GetParentScope() override
	{
		return m_pPackage->GetParentScope();
	}
	inline virtual void GetBaseScopes(std::vector<AST::Scope*>& bases) override
	{
		bases.push_back(dynamic_cast<Scope*>(this));
	}
	virtual bool Init(int varNum) override
	{
		return true;
	}
};
}
}