/*
Copyright (C) 2024 The XLang Foundation
Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

#pragma once
#include "object.h"
#include "scope.h"
#include "stackframe.h"
#include "xproxy.h"

namespace X
{
	class RemoteObject :
		public virtual XRemoteObject,
		public virtual X::XCustomScope,
		public virtual Data::Object,
		public virtual AST::Expression
	{
		//we use m_proxy as a weak reference, so we need to lock it
		Locker m_proxyLock;
		XProxy* m_proxy = nullptr;

		ROBJ_ID m_remote_Parent_Obj_id={0,0};
		ROBJ_ID m_remote_Obj_id = { 0,0 };
		AST::StackFrame* m_stackFrame = nullptr;
		std::string m_objName;
		ROBJ_MEMBER_ID m_memmberId = -1;
		int m_memberFlags = 0;

		//for remote object's members cache
		std::unordered_map <std::string, int> m_NameToIndex;

		FORCE_INLINE virtual int GetMemberFlags() override
		{
			return m_memberFlags;
		}
		FORCE_INLINE int NameToIndex(std::string& name, bool bGetOnly)
		{
			auto it = m_NameToIndex.find(name);
			if (it != m_NameToIndex.end())
			{
				return it->second;
			}
			else if (!bGetOnly)
			{
				int idx = (int)m_NameToIndex.size();
				m_NameToIndex.emplace(std::make_pair(name, idx));
				if (m_stackFrame)
				{
					m_stackFrame->SetVarCount(m_NameToIndex.size());
				}
				return idx;
			}
			else
			{
				return (int)AST::ScopeVarIndex::INVALID;
			}
		}
		FORCE_INLINE int GetNameCacheNum()
		{
			return (int)m_NameToIndex.size();
		}
	public:
		//if XProxy is null, means this process just keep this RemoteObject
		//will not convert to native object because this RemoteObject is not
		//inside this Process
		RemoteObject(XProxy* p):
			ObjRef(),XObj(),Object()
		{
			m_proxy = p;
			m_t = ObjType::RemoteObject;
			m_stackFrame = new AST::StackFrame();

			m_pMyScope = new AST::Scope();
			m_pMyScope->SetType(AST::ScopeType::RemoteObject);
			m_pMyScope->SetExp(this);
			m_pMyScope->SetDynScope(this);

		}
		~RemoteObject()
		{
			m_proxyLock.Lock();
			if (m_proxy)
			{
				m_proxy->RemoveOject(this);
			}
			m_proxyLock.Unlock();
			delete m_stackFrame;
		}
		std::string& GetObjName()
		{
			return m_objName;
		}
		void SetProxy(XProxy* p)
		{
			if (p != nullptr)
			{
				p->AddObject(this);
			}
			m_proxyLock.Lock();
			m_proxy = p;
			m_proxyLock.Unlock();
		}
		FORCE_INLINE bool IsValid()
		{
			bool bValid = false;
			m_proxyLock.Lock();
			if (m_proxy)
			{
				bValid = true;
			}
			m_proxyLock.Unlock();
			return bValid;
		}
		void SetObjName(std::string& name)
		{
			m_objName = name;
			if (m_remote_Obj_id.objId == nullptr)
			{
				m_proxyLock.Lock();
				if (m_proxy != nullptr)
				{
					m_remote_Obj_id = m_proxy->QueryRootObject(name);
				}
				m_proxyLock.Unlock();
			}
		}
		FORCE_INLINE ROBJ_ID GetObjId()
		{
			return m_remote_Obj_id;
		}
		FORCE_INLINE virtual int DecRef()
		{
			Lock();
			int ref = Ref();
			m_proxyLock.Lock();
			if ((m_proxy!=nullptr) && (ref == 1))
			{
				m_proxy->ReleaseObject(m_remote_Obj_id);
			}
			m_proxyLock.Unlock();
			Unlock();

			return Data::Object::DecRef();
		}

		virtual void GetBaseScopes(std::vector<AST::Scope*>& bases)
		{
			Object::GetBaseScopes(bases);
			bases.push_back(m_pMyScope);
		}
		virtual void SetObjID(unsigned long pid, void* objid)
		{
			m_remote_Obj_id = {pid,objid};
		}
		virtual bool CalcCallables(XlangRuntime* rt, XObj* pContext,
			std::vector<AST::Scope*>& callables)
		{
			return false;
		}
		virtual AST::Scope* GetScope()
		{
			return m_pMyScope;
		}
		virtual void ScopeLayout() override
		{
		}
		virtual bool ToBytes(XlangRuntime* rt, XObj* pContext, X::XLangStream& stream) override
		{
			AutoLock autoLock(Object::m_lock);
			Object::ToBytes(rt, pContext, stream);
			stream << m_remote_Parent_Obj_id;
			stream << m_remote_Obj_id;
			return true;
		}
		virtual bool FromBytes(X::XLangStream& stream) override
		{
			//this case, this object from cache, so we don't need to restore from stream
			//for some case in an application's root object, but this object also plays as
			//a remote object in another process.
			if (m_remote_Obj_id.objId !=0)
			{
				return true;
			}
			AutoLock autoLock(Object::m_lock);
			stream >> m_remote_Parent_Obj_id;
			stream >> m_remote_Obj_id;
			return true;
		}
		virtual bool Add(const X::Value& r, X::Value& retVal) override
		{
			auto* rt = g_pXHost->GetCurrentRuntime();
			bool bOK = SetValue((XlangRuntime*)rt, this, (X::Value&)r);
			retVal = Value(this);
			return bOK;
		}
		FORCE_INLINE bool SetValue(XlangRuntime* rt, XObj* pContext,X::Value& val)
		{
			X::Value retValue;
			ARGS params(1);
			params.push_back(val);
			KWARGS kwParams;
			X::Value varSetValue(true);
			kwParams.Add("SetValue", varSetValue);
			X::Value dummyTrailer;
			m_proxyLock.Lock();
			bool bOK = m_proxy?m_proxy->Call(rt, pContext,
				m_remote_Parent_Obj_id,
				m_remote_Obj_id, m_memmberId,
				params, kwParams, dummyTrailer, retValue):false;
			m_proxyLock.Unlock();
			return bOK;
		}
		virtual long long Size() override
		{
			m_proxyLock.Lock();
			auto retSize = m_proxy?m_proxy->QueryMemberCount(m_remote_Obj_id):0;
			m_proxyLock.Unlock();
			return retSize;
		}
		virtual X::Data::List* FlatPack(XlangRuntime* rt, XObj* pContext,
			std::vector<std::string>& IdList, int id_offset,
			long long startIndex, long long count) override;
		virtual X::Value UpdateItemValue(XlangRuntime* rt, XObj* pContext,
			std::vector<std::string>& IdList, int id_offset,
			std::string itemName, X::Value& val) override;
		virtual bool Call(XRuntime* rt, XObj* pContext, ARGS& params,
			KWARGS& kwParams,
			X::Value& retValue) override
		{
			bool keepRawParams = IS_KEEP_RAW_PARAMS(m_memberFlags);
			//if keep m_KeepRawParams is true, the last params
			//as trailer
			bool bOK = false;
			if (keepRawParams && params.size() >0)
			{
				ARGS params0(params.size() - 1);
				for (int i = 0; i < (params.size() - 1);i++)
				{
					params0.push_back(params[i]);
				}
				KWARGS kwParams0;
				m_proxyLock.Lock();
				bOK = m_proxy?m_proxy->Call(rt, pContext,
					m_remote_Parent_Obj_id,
					m_remote_Obj_id, m_memmberId,
					params0, kwParams0, params[params.size() - 1], retValue):false;
				m_proxyLock.Unlock();
			}
			else
			{
				X::Value dummyTrailer;
				m_proxyLock.Lock();
				bOK = m_proxy?m_proxy->Call(rt, pContext,
					m_remote_Parent_Obj_id,
					m_remote_Obj_id, m_memmberId,
					params, kwParams, dummyTrailer, retValue):false;
				m_proxyLock.Unlock();
			}
			return bOK;
		}
		virtual bool CallEx(XRuntime* rt, XObj* pContext,
			ARGS& params,
			KWARGS& kwParams,
			X::Value& trailer,
			X::Value& retValue)
		{
			m_proxyLock.Lock();
			bool bOK = m_proxy?m_proxy->Call(rt, pContext,
				m_remote_Parent_Obj_id,
				m_remote_Obj_id, m_memmberId,
				params, kwParams, trailer,retValue):false;
			m_proxyLock.Unlock();
			return bOK;
		}
		FORCE_INLINE virtual int AddOrGet(const char* name, bool bGetOnly) override final
		{
			std::string strName(name);
			int idx = NameToIndex(strName, true);//1/8/2024 change to true from bGetOnly
			//we need to check if this is a cache there, if not, go to remote to get it
			if (idx == (int)X::AST::ScopeVarIndex::INVALID)
			{
				int memberFlags = 0;
				m_proxyLock.Lock();
				X::ROBJ_MEMBER_ID memId = 0;
				if (m_proxy != nullptr)
				{
					memId = m_proxy->QueryMember(m_remote_Obj_id,strName, memberFlags);
				}
				m_proxyLock.Unlock();
				if (memId != -1)
				{
					idx = NameToIndex(strName, false);

					ROBJ_ID objId;
					m_proxyLock.Lock();
					if (m_proxy)
					{
						objId = m_proxy->GetMemberObject(m_remote_Obj_id, memId);
					}
					auto* r_obj = new RemoteObject(m_proxy);
					m_proxyLock.Unlock();

					r_obj->m_remote_Parent_Obj_id = m_remote_Obj_id;
					r_obj->m_remote_Obj_id = objId;
					r_obj->m_memmberId = memId;
					r_obj->m_objName = name;
					r_obj->m_memberFlags = memberFlags;
					//r_obj->Object::IncRef();
					Value valObj(dynamic_cast<XObj*>(r_obj));
					m_stackFrame->Set(idx, valObj);
				}
			}
			return idx;
		}
		FORCE_INLINE virtual bool Set(int idx, X::Value& v) override final
		{
			m_stackFrame->Set(idx, v);
			return true;
		}
		FORCE_INLINE virtual bool Get(int idx, X::Value& v, void* lValue = nullptr) override final
		{
			m_stackFrame->Get(idx, v, (X::LValue*)lValue);
			return true;
		}
	};
}