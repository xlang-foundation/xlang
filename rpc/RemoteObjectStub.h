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

#include "singleton.h"
#include "service_def.h"
#include <unordered_map>
#include "Locker.h"
#include "StubMgr.h"
#include "xproxy.h"
#include "xhost.h"
#include "xlang.h"

namespace X
{
	class RemoteObjectStub :
		public RemotingCallBase,
		public Singleton<RemoteObjectStub>
	{
	public:
		RemoteObjectStub();
		~RemoteObjectStub();
		void Register();
		bool ExtractNativeObjectFromRemoteObject(
			X::Value& remoteObj,
			X::Value& nativeObj);
	private:
		X::XRuntime* m_rt = nullptr;
		void EnsureRT();
		X::XObj* QueryObjWithName(std::string& name);
		X::XObj* CovertIdToXObj(X::ROBJ_ID);
		X::ROBJ_ID ConvertXObjToId(X::XObj* obj);
		bool QueryRootObject(void* pCallContext,SwapBufferStream& stream, RemotingProc* pProc);
		bool QueryMember(void* pCallContext, SwapBufferStream& stream, RemotingProc* pProc);
		bool QueryMemberCount(void* pCallContext, SwapBufferStream& stream, RemotingProc* pProc);
		bool FlatPack(void* pCallContext, SwapBufferStream& stream, RemotingProc* pProc);
		bool UpdateItemValue(void* pCallContext, SwapBufferStream& stream, RemotingProc* pProc);
		bool GetMemberObject(void* pCallContext, SwapBufferStream& stream, RemotingProc* pProc);
		bool ReleaseObject(void* pCallContext, SwapBufferStream& stream, RemotingProc* pProc);
		bool RCall(void* pCallContext, SwapBufferStream& stream, RemotingProc* pProc);
		// Inherited via RemotingCallBase
		virtual bool Call(void* pCallContext,unsigned int callType,SwapBufferStream& stream, RemotingProc* pProc) override;
	};
}