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