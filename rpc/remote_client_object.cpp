#include "remote_client_object.h"
#include "StubMgr.h"

namespace X
{
	bool RemoteClientObject::Call(XRuntime* rt, XObj* pContext, ARGS& params,
		KWARGS& kwParams,
		X::Value& retValue)
	{
		if (m_stub == nullptr)
		{
			return false;
		}
		//we need to check if this stub is still valid by session id
		bool isStubSessionValid = RemotingManager::I().IsStubExist(m_stubSessionId);
		if (!isStubSessionValid)
		{
			m_stub = nullptr;
			return false;
		}
		bool bOK = m_stub->CallClient(m_remote_Obj_id, params, kwParams);
		retValue = X::Value(bOK);
		return bOK;
	}
}