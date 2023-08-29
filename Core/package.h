#pragma once
#include "exp.h"
#include "object.h"
#include "stackframe.h"
#include <functional>
#include "Locker.h"

namespace X
{
namespace AST
{
	struct MemberIndexInfo
	{
		std::string name;
		std::string doc;
		PackageMemberType type;
		int Index = -1;
		bool KeepRawParams =false;//treat last parameter as bytes, don't do decoding
	};
class Package :
	virtual public XPackage,
	virtual public Data::Object,
	virtual public Scope
{
	//Meta data for this package
	void* m_apiset = nullptr;

	void* m_pObject = nullptr;
	StackFrame* m_stackFrame = nullptr;
	std::vector<MemberIndexInfo> m_memberInfos;
	PackageCleanup m_funcPackageCleanup = nullptr;
	PackageWaitFunc m_funcPackageWait = nullptr;
	PackageAccessor m_funcAccessor;

	virtual void SetPackageAccessor(PackageAccessor func) override
	{
		m_funcAccessor = func;
	}
	virtual void SetPackageCleanupFunc(PackageCleanup func) override
	{
		m_funcPackageCleanup = func;
	}
	virtual void SetPackageWaitFunc(PackageWaitFunc func) override
	{
		m_funcPackageWait = func;
	}
	virtual bool IsSamePackage(XPackage* pPack) override
	{
		Package* pPackage = dynamic_cast<Package*>(pPack);
		return (pPackage->m_apiset == m_apiset);
	}
	std::vector<AST::Module*> m_loadedModules;//for adding xlang code
	void UnloadAddedModules();
	Locker m_lock;
public:
	inline std::vector<MemberIndexInfo>& GetMemberInfo() { return m_memberInfos; }
	Package(void* pObj):
		Data::Object(), Scope()
	{
		m_pObject = pObj;
		m_t = X::ObjType::Package;
	}
	~Package()
	{
		UnloadAddedModules();
		if (m_pObject)
		{
			Cleanup(m_pObject);
		}
	}
	virtual void SetAPISet(void* pApiSet) override
	{
		m_apiset = pApiSet;
	}
	inline void* GetAPISet()
	{
		return m_apiset;
	}
	inline virtual bool wait(int timeout) override
	{
		bool bOK = true;
		if (m_funcPackageWait)
		{
			bOK = m_funcPackageWait(m_pObject, timeout);
		}
		return bOK;
	}
	PackageAccessor GetAccessor()
	{
		return m_funcAccessor;
	}
	PackageWaitFunc GetWaitFunc()
	{
		return m_funcPackageWait;
	}
	void Lock()
	{
		m_lock.Lock();
	}
	void Unlock()
	{
		m_lock.Unlock();
	}
	bool ToBytesImpl(XlangRuntime* rt, void* pEmbededObject, X::XLangStream& stream);
	bool FromBytesImpl(void* pEmbededObject, X::XLangStream& stream);
	inline virtual bool ToBytes(XlangRuntime* rt, XObj* pContext, X::XLangStream& stream) override
	{
		return ToBytesImpl(rt,m_pObject, stream);
	}
	inline virtual bool FromBytes(X::XLangStream& stream) override
	{
		return FromBytesImpl(m_pObject, stream);
	}
	virtual bool RunCodeWithThisScope(const char* code) override;
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
	inline virtual int AddMember(PackageMemberType type, const char* name,
		const char* doc, bool keepRawParams = false) override
	{
		std::string strName(name);
		std::string strDoc(doc);
		int idx =  Scope::AddOrGet(strName, false);
		int size = (int)m_memberInfos.size();
		if (size <= idx)
		{
			for (int i = size; i < idx; i++)
			{
				m_memberInfos.push_back({});
			}
			m_memberInfos.push_back({ strName,strDoc,type,idx,keepRawParams });
		}
		else
		{
			m_memberInfos[idx] = { strName,strDoc,type,idx,keepRawParams };
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
	virtual int AddOrGet(std::string& name, bool bGetOnly, Scope** ppRightScope = nullptr) override
	{
		//in this case, can't add new member, in init stage, will call Scope::AddOrGet directly
		//add member
		return Scope::AddOrGet(name, true, ppRightScope);
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
		Object::GetBaseScopes(bases);
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
	virtual void SetPackageWaitFunc(PackageWaitFunc func) override
	{
	}
	virtual bool RunCodeWithThisScope(const char* code) override
	{
		return true;
	}
	virtual bool IsSamePackage(XPackage* pPack) override
	{
		Package* pPackage = dynamic_cast<Package*>(pPack);
		return (m_pPackage->GetAPISet() == pPackage->GetAPISet());
	}
public:
	Package* GetPackage() { return m_pPackage; }
	virtual void SetPackageAccessor(PackageAccessor func) override
	{
	}
	inline virtual bool wait(int timeout) override
	{
		bool bOK = true;
		if (m_pPackage->GetWaitFunc())
		{
			bOK = m_pPackage->GetWaitFunc()(m_pObject, timeout);
		}
		return bOK;
	}
	PackageProxy(Package* pPack,void* pObj) :
		Data::Object(), Scope()
	{
		m_pPackage = pPack;
		if (m_pPackage)
		{
			m_pPackage->Scope::IncRef();
			m_stackFrame = new StackFrame(this);
			//? multiple threads will cause crash
			//so lock here to try
			//todo: check here, was commented out line below, shawn@4/21/2023
			m_pPackage->Lock();
			auto* pBaseStack = m_pPackage->GetStack();
			int cnt = pBaseStack->GetVarCount();
			m_stackFrame->SetVarCount(cnt);
			for (int i = 0; i < cnt; i++)
			{
				X::Value v0;
				pBaseStack->Get(i, v0);
				if (v0.IsObject() && v0.GetObj()->GetType() == ObjType::Prop)
				{
					//continue;
					//v0.GetObj()->IncRef();
				}
				if (v0.IsObject() && v0.GetObj()->GetType() == ObjType::ObjectEvent)
				{
					v0.Clone();
				}
				m_stackFrame->Set(i, v0);
			}
			m_pPackage->Unlock();
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
		//m_pPackage->Lock();
		if (m_stackFrame)
		{
			delete m_stackFrame;
			m_stackFrame = nullptr;
		}
		//m_pPackage->Unlock();
	}
	virtual void SetAPISet(void* pApiSet) override {}
	inline virtual bool ToBytes(XlangRuntime* rt, XObj* pContext, X::XLangStream& stream) override
	{
		return m_pPackage ? m_pPackage->ToBytesImpl(rt, m_pObject, stream) : false;
	}
	inline virtual bool FromBytes(X::XLangStream& stream) override
	{
		return m_pPackage ? m_pPackage->FromBytesImpl(m_pObject, stream) : false;
	}
	virtual X::Data::List* FlatPack(XlangRuntime* rt, XObj* pContext,
		std::vector<std::string>& IdList, int id_offset,
		long long startIndex, long long count) override;
	virtual X::Value UpdateItemValue(XlangRuntime* rt, XObj* pContext,
		std::vector<std::string>& IdList, int id_offset,
		std::string itemName, X::Value& val) override;
	inline virtual bool Get(XRuntime* rt, XObj* pContext, X::Port::vector<X::Value>& IdxAry,X::Value& val)override
	{
		if (m_pPackage)
		{
			auto func = m_pPackage->GetAccessor();
			if (func)
			{
				val = func(rt, pContext,IdxAry);
				return true;
			}
		}
		return false;
	}
	virtual long long Size()
	{
		return m_pPackage->Size();
	}
	virtual int AddOrGet(std::string& name, bool bGetOnly, Scope** ppRightScope = nullptr) override
	{
		return m_pPackage->AddOrGet(name, bGetOnly);
	}
	virtual void RemoveALl() override
	{
		m_pPackage->RemoveALl();
		//m_pPackage->Lock();
		if (m_stackFrame)
		{
			delete m_stackFrame;
			m_stackFrame = nullptr;
		}
		//m_pPackage->Unlock();
	}
	inline virtual int AddMember(PackageMemberType type,const char* name,const char* doc,bool keepRawParams =false) override
	{
		return m_pPackage->AddMember(type,name,doc,keepRawParams);
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