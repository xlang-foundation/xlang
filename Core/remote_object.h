#pragma once
#include "object.h"
#include "scope.h"
#include "stackframe.h"
#include "xproxy.h"

namespace X
{
	class RemoteObject :
		public virtual XRemoteObject,
		public virtual AST::Scope,
		public virtual Data::Object,
		public virtual AST::Expression
	{
		XProxy* m_proxy = nullptr;
		ROBJ_ID m_remote_Parent_Obj_id={0,0};
		ROBJ_ID m_remote_Obj_id = { 0,0 };
		AST::StackFrame* m_stackFrame = nullptr;
		std::string m_objName;
		ROBJ_MEMBER_ID m_memmberId = -1;
		bool m_KeepRawParams = false;
	public:
		//if XProxy is null, means this process just keep this RemoteObject
		//will not convert to native object because this RemoteObject is not
		//inside this Process
		RemoteObject(XProxy* p):
			ObjRef(),XObj(),Scope(), Object()
		{
			m_proxy = p;
			m_t = ObjType::RemoteObject;
			m_stackFrame = new AST::StackFrame(this);
		}
		~RemoteObject()
		{
			delete m_stackFrame;
		}
		void SetObjName(std::string& name)
		{
			m_objName = name;
			if (m_remote_Obj_id.objId == nullptr)
			{
				m_remote_Obj_id = m_proxy->QueryRootObject(name);
			}
		}
		inline ROBJ_ID GetObjId()
		{
			return m_remote_Obj_id;
		}
		inline virtual int DecRef()
		{
			Lock();
			int ref = Ref();
			if ((m_proxy!=nullptr) && (ref == 1))
			{
				m_proxy->ReleaseObject(m_remote_Obj_id);
			}
			Unlock();

			return Data::Object::DecRef();;
		}
		virtual void GetBaseScopes(std::vector<AST::Scope*>& bases)
		{
			bases.push_back(dynamic_cast<Scope*>(this));
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
			return this;
		}
		virtual void ScopeLayout() override
		{
		}
		virtual bool ToBytes(XlangRuntime* rt, XObj* pContext, X::XLangStream& stream) override
		{
			AutoLock(m_lock);
			Object::ToBytes(rt, pContext, stream);
			stream << m_remote_Parent_Obj_id;
			stream << m_remote_Obj_id;
			return true;
		}
		virtual bool FromBytes(X::XLangStream& stream) override
		{
			AutoLock(m_lock);
			stream >> m_remote_Parent_Obj_id;
			stream >> m_remote_Obj_id;
			return true;
		}
		virtual Scope* GetParentScope() override
		{
			return nullptr;
		}
		bool SetValue(XlangRuntime* rt, XObj* pContext,X::Value& val)
		{
			X::Value retValue;
			ARGS params;
			params.push_back(val);
			KWARGS kwParams;
			X::Value dummyTrailer;
			return m_proxy->Call(rt, pContext,
				m_remote_Parent_Obj_id,
				m_remote_Obj_id, m_memmberId,
				params, kwParams, dummyTrailer, retValue);
		}
		virtual long long Size() override
		{
			return m_proxy->QueryMemberCount(m_remote_Obj_id);
		}
		virtual X::Data::List* FlatPack(XlangRuntime* rt, XObj* pContext,
			std::vector<std::string>& IdList, int id_offset,
			long long startIndex, long long count) override;
		virtual bool Call(XRuntime* rt, XObj* pContext, ARGS& params,
			KWARGS& kwParams,
			X::Value& retValue) override
		{
			//if keep m_KeepRawParams is true, the last params
			//as trailer
			if (m_KeepRawParams && params.size() >0)
			{
				ARGS params0;
				for (int i = 0; i < (params.size() - 1);i++)
				{
					params0.push_back(params[i]);
				}
				KWARGS kwParams0;
				return m_proxy->Call(rt, pContext,
					m_remote_Parent_Obj_id,
					m_remote_Obj_id, m_memmberId,
					params0, kwParams0, params[params.size() - 1], retValue);
			}
			else
			{
				X::Value dummyTrailer;
				return m_proxy->Call(rt, pContext,
					m_remote_Parent_Obj_id,
					m_remote_Obj_id, m_memmberId,
					params, kwParams, dummyTrailer, retValue);
			}
		}
		virtual bool CallEx(XRuntime* rt, XObj* pContext,
			ARGS& params,
			KWARGS& kwParams,
			X::Value& trailer,
			X::Value& retValue)
		{
			return m_proxy->Call(rt, pContext,
				m_remote_Parent_Obj_id,
				m_remote_Obj_id, m_memmberId,
				params, kwParams, trailer,retValue);
		}
		virtual int AddOrGet(std::string& name, bool bGetOnly) override
		{
			int idx = Scope::AddOrGet(name, bGetOnly);
			if (idx == (int)X::AST::ScopeVarIndex::INVALID)
			{
				bool KeepRawParams = false;
				auto memId = m_proxy->QueryMember(m_remote_Obj_id, name, KeepRawParams);
				if (memId != -1)
				{
					auto objId = m_proxy->GetMemberObject(m_remote_Obj_id, memId);
					idx = Scope::AddOrGet(name, false);
					m_stackFrame->SetVarCount(GetVarNum());

					auto* r_obj = new RemoteObject(m_proxy);
					r_obj->m_remote_Parent_Obj_id = m_remote_Obj_id;
					r_obj->m_remote_Obj_id = objId;
					r_obj->m_memmberId = memId;
					r_obj->m_objName = name;
					r_obj->m_KeepRawParams = KeepRawParams;
					//r_obj->Object::IncRef();
					Value valObj(dynamic_cast<XObj*>(r_obj));
					m_stackFrame->Set(idx, valObj);
				}
			}
			return idx;
		}
		inline virtual bool Set(XlangRuntime* rt, XObj* pContext,
			int idx, X::Value& v) override
		{
			m_stackFrame->Set(idx, v);
			return true;
		}
		inline virtual bool Get(XlangRuntime* rt, XObj* pContext,
			int idx, X::Value& v, LValue* lValue = nullptr) override
		{
			m_stackFrame->Get(idx, v, lValue);
			return true;
		}
	};
}