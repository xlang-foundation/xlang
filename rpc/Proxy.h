#pragma once
#include "singleton.h"
#include "xproxy.h"
#include "SwapBufferStream.h"
#include "Locker.h"
#include "gthread.h"

class XWait;
namespace X
{
	class XLangProxyManager:
		public Singleton<XLangProxyManager>
	{
	public:
		void Register();
	};
	class SMSwapBuffer;
	class XLangProxy :
		public X::XProxy,
		public GThread2
	{
	public:
		XLangProxy();
		~XLangProxy();
		virtual ROBJ_ID QueryRootObject(std::string& name);
		virtual X::ROBJ_MEMBER_ID QueryMember(X::ROBJ_ID id, std::string& name,
			bool& KeepRawParams);
		virtual X::ROBJ_ID GetMemberObject(X::ROBJ_ID objid, X::ROBJ_MEMBER_ID memId);
		virtual bool Call(XRuntime* rt, XObj* pContext,
			X::ROBJ_ID parent_id, X::ROBJ_ID id, X::ROBJ_MEMBER_ID memId,
			X::ARGS& params, X::KWARGS& kwParams, X::Value& trailer,X::Value& retValue);
		// GThread interface
	protected:
		bool mRun = true;
		virtual void run() override;
		virtual void run2() override;
	private:
		unsigned long mHostProcessId = 0;
		unsigned long long mSessionId = 0;
		bool m_ExitOnHostExit = false;
		bool m_Exited = false;
		void WaitToHostExit();

		Locker m_ConnectLock;
		bool m_bConnected = false;
		XWait* m_pConnectWait = nullptr;

		SwapBufferStream mStream1;
		SMSwapBuffer* mSMSwapBuffer1 = nullptr;
		SwapBufferStream mStream2;
		SMSwapBuffer* mSMSwapBuffer2 = nullptr;

		Locker mCallLock1;
		SwapBufferStream& BeginCall(unsigned int callType);
		SwapBufferStream& CommitCall();
		void FinishCall();

		Locker mCallLock2;
		SwapBufferStream& BeginCall2(unsigned int callType);
		SwapBufferStream& CommitCall2();
		void FinishCall2();

		bool CheckConnectReadyStatus();
		bool Connect();
		void Disconnect();
		void ShapeHandsToServer();
	};
}