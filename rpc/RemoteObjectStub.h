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