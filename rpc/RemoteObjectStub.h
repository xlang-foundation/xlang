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
	private:
		X::XRuntime* m_rt = nullptr;
		void EnsureRT();
		X::XObj* QueryObjWithName(std::string& name);
		X::XObj* CovertIdToXObj(X::ROBJ_ID);
		X::ROBJ_ID ConvertXObjToId(X::XObj* obj);
		bool QueryRootObject(int channel, SwapBufferStream& stream, RemotingProc* pProc);
		bool QueryMember(int channel, SwapBufferStream& stream, RemotingProc* pProc);
		bool GetMemberObject(int channel, SwapBufferStream& stream, RemotingProc* pProc);
		bool RCall(int channel, SwapBufferStream& stream, RemotingProc* pProc);
		// Inherited via RemotingCallBase
		virtual bool Call(int channel, unsigned int callId,
			SwapBufferStream& stream, RemotingProc* pProc) override;
	};
}