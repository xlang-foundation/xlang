#pragma once
#include "object.h"
#include "Stub.h"

namespace X
{
	class RemoteClientObject :
		public virtual Data::Object
	{
		ROBJ_ID m_remote_Obj_id = { 0,0 };
		XLangStub* m_stub = nullptr;
	public:
		RemoteClientObject(X::ROBJ_ID robjId) :
			ObjRef(), Object()
		{
			m_t = ObjType::RemoteClientObject;
			m_remote_Obj_id = robjId;
		}
		RemoteClientObject(XLangStub* stub) :
			ObjRef(), Object()
		{
			m_stub = stub;
			m_t = ObjType::RemoteClientObject;
		}
		void SetStub(XLangStub* stub)
		{
			m_stub = stub;
		}
		~RemoteClientObject()
		{
		}
		virtual bool ToBytes(XlangRuntime* rt, XObj* pContext, X::XLangStream& stream) override
		{
			AutoLock autoLock(Object::m_lock);
			Object::ToBytes(rt, pContext, stream);
			stream << m_remote_Obj_id;
			return true;
		}
		virtual bool FromBytes(X::XLangStream& stream) override
		{
			AutoLock autoLock(Object::m_lock);
			stream >> m_remote_Obj_id;
			return true;
		}
		virtual bool Call(XRuntime* rt, XObj* pContext, ARGS& params,
			KWARGS& kwParams,
			X::Value& retValue) override
		{
			bool bOK = m_stub->CallClient(m_remote_Obj_id, params, kwParams);
			retValue = X::Value(bOK);
			return bOK;
		}
	};
}