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
	virtual public Data::Object
{
	AST::Scope* m_pMyScope = nullptr;
	//Meta data for this package
	void* m_apiset = nullptr;

	void* m_pObject = nullptr;
	StackFrame* m_variableFrame = nullptr;
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
	FORCE_INLINE std::vector<MemberIndexInfo>& GetMemberInfo() { return m_memberInfos; }
	Package(void* pObj):
		Data::Object()
	{
		m_pObject = pObj;
		m_t = X::ObjType::Package;
		m_pMyScope = new Scope();
		m_pMyScope->SetType(ScopeType::Package);
	}
	~Package()
	{
		UnloadAddedModules();
		if (m_pObject)
		{
			Cleanup(m_pObject);
		}
		delete m_pMyScope;
	}
	virtual void SetAPISet(void* pApiSet) override
	{
		m_apiset = pApiSet;
	}
	FORCE_INLINE void* GetAPISet()
	{
		return m_apiset;
	}
	FORCE_INLINE virtual bool wait(int timeout) override
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
	FORCE_INLINE virtual bool ToBytes(XlangRuntime* rt, XObj* pContext, X::XLangStream& stream) override
	{
		return ToBytesImpl(rt,m_pObject, stream);
	}
	FORCE_INLINE virtual bool FromBytes(X::XLangStream& stream) override
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
		if (m_variableFrame)
		{
			delete m_variableFrame;
			m_variableFrame = nullptr;
		}
	}
	StackFrame* GetStack() { return m_variableFrame; }
	FORCE_INLINE void Cleanup(void* pObj)
	{
		if (m_funcPackageCleanup)
		{
			m_funcPackageCleanup(pObj);
		}
	}
	FORCE_INLINE virtual int AddMember(PackageMemberType type, const char* name,
		const char* doc, bool keepRawParams = false) override
	{
		std::string strName(name);
		std::string strDoc(doc);
		int idx =  m_pMyScope->AddOrGet(strName, false);
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
	FORCE_INLINE virtual int QueryMethod(const char* name, bool* pKeepRawParams = nullptr) override
	{
		std::string strName(name);
		int idx =  m_pMyScope->AddOrGet(strName, true);
		if (idx>=0 && pKeepRawParams)
		{
			*pKeepRawParams = m_memberInfos[idx].KeepRawParams;
		}
		return idx;
	}
	FORCE_INLINE virtual MemberIndexInfo QueryMethod(std::string name)
	{
		int idx =  m_pMyScope->AddOrGet(name, true);
		return m_memberInfos[idx];
	}

	FORCE_INLINE virtual AST::Scope* GetMyScope() override
	{
		return m_pMyScope;
	}
	virtual void* GetEmbedObj() override 
	{ 
		return m_pObject; 
	}
	virtual bool Init(int varNum) override
	{
		m_variableFrame = new StackFrame();
		m_variableFrame->SetVarCount(varNum);
		m_pMyScope->SetVarFrame(m_variableFrame);
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
	bool Set(XlangRuntime* rt, XObj* pContext, int idx, Value& v)
	{
		m_variableFrame->Set(idx, v);
		return true;
	}
	virtual bool SetIndexValue(int idx, Value& v) override
	{
		m_variableFrame->Set(idx, v);
		return true;
	}
	virtual bool GetIndexValue(int idx, Value& v)
	{
		m_variableFrame->Get(idx,v);
		return true;
	}
	bool Get(XlangRuntime* rt, XObj* pContext, int idx, Value& v,
		LValue* lValue = nullptr)
	{
		m_variableFrame->Get(idx, v, lValue);
		return true;
	}
	virtual void GetBaseScopes(std::vector<AST::Scope*>& bases) override
	{
		Object::GetBaseScopes(bases);
		bases.push_back(dynamic_cast<Scope*>(m_pMyScope));
	}
};
class PackageProxy :
	virtual public XPackage,
	virtual public Data::Object
{
	AST::Scope* m_pMyScope = nullptr;
	void* m_pObject = nullptr;
	//for functions, just addref
	//for event, need to clone a new event
	//for prop, TODO:
	//all add into m_variableFrame
	StackFrame* m_variableFrame = nullptr;
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
	FORCE_INLINE virtual bool wait(int timeout) override
	{
		bool bOK = true;
		if (m_pPackage->GetWaitFunc())
		{
			bOK = m_pPackage->GetWaitFunc()(m_pObject, timeout);
		}
		return bOK;
	}
	PackageProxy(Package* pPack,void* pObj) :
		Data::Object()
	{
		m_pPackage = pPack;
		m_pMyScope = new Scope();
		m_pMyScope->SetType(ScopeType::Package);
		if (m_pPackage)
		{
			m_variableFrame = new StackFrame();
			m_pMyScope->SetVarFrame(m_variableFrame);
			m_pMyScope->SetNamespaceScope(m_pPackage->GetMyScope());
			//? multiple threads will cause crash
			//so lock here to try
			//todo: check here, was commented out line below, shawn@4/21/2023
			m_pPackage->Lock();
			auto* pBaseStack = m_pPackage->GetStack();
			int cnt = pBaseStack->GetVarCount();
			m_variableFrame->SetVarCount(cnt);
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
				m_variableFrame->Set(i, v0);
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
		//m_pPackage->Lock();
		if (m_variableFrame)
		{
			delete m_variableFrame;
			m_variableFrame = nullptr;
		}
		//m_pPackage->Unlock();

		delete m_pMyScope;
	}
	FORCE_INLINE virtual AST::Scope* GetMyScope() override
	{
		return m_pMyScope;
	}
	virtual void SetAPISet(void* pApiSet) override {}
	FORCE_INLINE virtual bool ToBytes(XlangRuntime* rt, XObj* pContext, X::XLangStream& stream) override
	{
		return m_pPackage ? m_pPackage->ToBytesImpl(rt, m_pObject, stream) : false;
	}
	FORCE_INLINE virtual bool FromBytes(X::XLangStream& stream) override
	{
		return m_pPackage ? m_pPackage->FromBytesImpl(m_pObject, stream) : false;
	}
	virtual X::Data::List* FlatPack(XlangRuntime* rt, XObj* pContext,
		std::vector<std::string>& IdList, int id_offset,
		long long startIndex, long long count) override;
	virtual X::Value UpdateItemValue(XlangRuntime* rt, XObj* pContext,
		std::vector<std::string>& IdList, int id_offset,
		std::string itemName, X::Value& val) override;
	FORCE_INLINE virtual bool Get(XRuntime* rt, XObj* pContext, X::Port::vector<X::Value>& IdxAry,X::Value& val)override
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
	virtual void RemoveALl() override
	{
		m_pPackage->RemoveALl();
		//m_pPackage->Lock();
		if (m_variableFrame)
		{
			delete m_variableFrame;
			m_variableFrame = nullptr;
		}
		//m_pPackage->Unlock();
	}
	FORCE_INLINE virtual int AddMember(PackageMemberType type,const char* name,const char* doc,bool keepRawParams =false) override
	{
		return m_pPackage->AddMember(type,name,doc,keepRawParams);
	}
	FORCE_INLINE virtual int QueryMethod(const char* name, bool* pKeepRawParams = nullptr) override
	{
		return m_pPackage->QueryMethod(name, pKeepRawParams);
	}
	FORCE_INLINE virtual MemberIndexInfo QueryMethod(std::string name)
	{
		return m_pPackage->QueryMethod(name);
	}
	FORCE_INLINE virtual AST::Scope* GetScope()
	{
		return m_pPackage->GetMyScope();
	}
	virtual void* GetEmbedObj() override
	{
		return m_pObject;
	}
	virtual bool SetIndexValue(int idx, Value& v) override
	{
		m_variableFrame->Set(idx, v);
		return true;
	}
	virtual bool GetIndexValue(int idx, Value& v)
	{
		m_variableFrame->Get(idx, v);
		return true;
	}
	FORCE_INLINE virtual void GetBaseScopes(std::vector<AST::Scope*>& bases) override
	{
		bases.push_back(dynamic_cast<Scope*>(m_pPackage->GetMyScope()));
	}
	virtual bool Init(int varNum) override
	{
		return true;
	}
};
}
}