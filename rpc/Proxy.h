#pragma once
#include "singleton.h"
#include "xproxy.h"
#include "SwapBufferStream.h"
#include "Locker.h"
#include "gthread.h"
#include "remote_client_object.h"
#include <vector>

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

	struct Call_Context
	{
		bool InUse = false;
		XWait* pWait = nullptr;
	};
	class XLangProxy :
		public X::XProxy,
		public GThread2
	{
		X::Value mOwnerObject;
		std::string mUrl;
		std::string mProxyName;
	public:
		XLangProxy();
		~XLangProxy();
		virtual void SetOwner(X::Value& ownerObject) override
		{
			mOwnerObject = ownerObject;
		}
		virtual void SetUrl(std::string& url) override;
		virtual void SetName(std::string& name) override
		{
			mProxyName = name;
		}
		virtual std::string GetUrl() override { return mUrl; }
		virtual ROBJ_ID QueryRootObject(std::string& name);
		virtual X::ROBJ_MEMBER_ID QueryMember(X::ROBJ_ID id, std::string& name,
			bool& KeepRawParams);
		virtual long long QueryMemberCount(X::ROBJ_ID id);
		virtual bool FlatPack(X::ROBJ_ID parentObjId, X::ROBJ_ID id,
			Port::vector<std::string>& IdList, int id_offset,
			long long startIndex, long long count, Value& retList);
		virtual X::Value UpdateItemValue(X::ROBJ_ID parentObjId, X::ROBJ_ID id,
			Port::vector<std::string>& IdList, int id_offset,
			std::string itemName, X::Value& val);
		virtual X::ROBJ_ID GetMemberObject(X::ROBJ_ID objid, X::ROBJ_MEMBER_ID memId);
		virtual bool ReleaseObject(ROBJ_ID id) override;
		virtual bool Call(XRuntime* rt, XObj* pContext,
			X::ROBJ_ID parent_id, X::ROBJ_ID id, X::ROBJ_MEMBER_ID memId,
			X::ARGS& params, X::KWARGS& kwParams, X::Value& trailer,X::Value& retValue);
		virtual void SetTimeout(int timeout) override
		{
			mTimeout = timeout;
		}
		// GThread interface
	protected:
		bool mRun = true;
		virtual void run() override;
		virtual void run2() override;

		Locker m_CallContextLock;
		std::vector<Call_Context*> mCallContexts;
		Call_Context* GetCallContext();
		FORCE_INLINE void ReturnCallContext(Call_Context* pContext)
		{
			m_CallContextLock.Lock();
			pContext->InUse = false;
			m_CallContextLock.Unlock();
		}
	private:
		int m_refCount = 0;
		Locker mLockRefCount;

		int mTimeout = -1;
		long m_port = 0;
		unsigned long mHostProcessId = 0;
		unsigned long long mSessionId = 0;
		bool m_ExitOnHostExit = true;
		bool m_Exited = false;
		void WaitToHostExit();

		Locker m_ConnectLock;
		bool m_bConnected = false;
		XWait* m_pConnectWait = nullptr;
		XWait* m_pCallReadyWait = nullptr;

		SwapBufferStream mStream1;
		SMSwapBuffer* mSMSwapBuffer1 = nullptr;
		SwapBufferStream mStream2;
		SMSwapBuffer* mSMSwapBuffer2 = nullptr;
		XWait* m_pBuffer2ReadyWait = nullptr;

		Locker mCallLock1;
		SwapBufferStream& BeginCall(unsigned int callType, Call_Context** ppContext);
		SwapBufferStream& CommitCall(Call_Context* pContext);
		void FinishCall();

		bool CheckConnectReadyStatus();
		bool Connect();
		void Disconnect();
		void ShapeHandsToServer();


		X::XObj* CovertIdToXObj(X::ROBJ_ID id)
		{
			X::XObj* pObjRet = nullptr;
			auto pid = GetPID();
			if (pid != id.pid)
			{
				//TODO::keep as RemoteObject
			}
			else
			{
				pObjRet = (X::XObj*)id.objId;
			}
			return pObjRet;
		}

		X::Value ConvertXObjToRemoteClientObject(X::XObj* obj)
		{
			obj->IncRef();
			auto pid = GetPID();
			X::ROBJ_ID robjId{ pid,obj };
			RemoteClientObject* pRC = new RemoteClientObject(robjId);
			X::Value val(pRC);
			return val;
		}
	};
}