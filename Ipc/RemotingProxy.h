#pragma once
#include "singleton.h"
#include "SwapBufferStream.h"
#include "Locker.h"
#include "gthread.h"
#include "remote_client_object.h"
#include "remote_object.h"
#include <vector>
#include "port.h"
#include "Call.h"

#define LRPC_NAME "lrpc"

namespace X
{
	namespace IPC
	{
		class RemotingProxyManager :
			public Singleton<RemotingProxyManager>
		{
		public:
			void Register();
		};
		class RemotingProxy;
		class ProxySessionThread :
			public GThread
		{
		public:
			void SetParent(RemotingProxy* p)
			{
				mParent = p;
			}
		protected:
			virtual void run() override;
		private:
			RemotingProxy* mParent = nullptr;
		};
		class RemotingProxy :
			public CallHandler
		{
			std::string mRootObjectName;
			ProxySessionThread mSessionThread;
		public:
			RemotingProxy();
			~RemotingProxy();
			virtual void SetRootObjectName(const char* name)
			{
				mRootObjectName = name;
			}
			virtual unsigned long long GetSessionId() override
			{
				return 0;
			}
			void StartProxy()
			{
				mSessionThread.SetParent(this);
				CallHandler::Start();
				mSessionThread.Start();
			}
			void SetUrl(std::string& url);
			virtual int AddRef() override
			{
				return CallHandler::AddRef();
			}
			virtual int Release() override
			{
				return CallHandler::Release();
			}
			virtual ROBJ_ID QueryRootObject(std::string& name);
			virtual X::ROBJ_MEMBER_ID QueryMember(X::ROBJ_ID id, std::string& name,
				int& memberFlags);
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
				X::ARGS& params, X::KWARGS& kwParams, X::Value& trailer, X::Value& retValue);
			virtual void SetTimeout(int timeout) override
			{
				mTimeout = timeout;
			}
			void SessionRun();

		protected:
			bool mRun = true;
			void Cleanup();

		private:
			bool mHostUseGlobal = false;


			int m_ThreadRefCount = 0;//we have two threads there
			Locker mLockRefCount;

			virtual int ThreadAddRef()
			{
				int ret = 0;
				mLockRefCount.Lock();
				ret = ++m_ThreadRefCount;
				mLockRefCount.Unlock();
				return ret;
			}
			virtual int ThreadRelease()
			{
				int ret = 0;
				mLockRefCount.Lock();
				ret = --m_ThreadRefCount;
				mLockRefCount.Unlock();
				if (ret == 0)
				{
					Cleanup();
				}
				return ret;
			}
			virtual void ShakeHandsCall(void* pCallContext, SwapBufferStream& stream) override
			{
			}
			int mTimeout = -1;
			long m_port = 0;
			unsigned long mHostProcessId = 0;
			unsigned long long mSessionId = 0;
			bool m_ExitOnHostExit = true;
			bool m_Exited = false;
			void WaitToHostExit();
			void WaitToHostExit_ByProcess();

			Locker m_ConnectLock;
			bool m_bConnected = false;
			XWait* m_pConnectWait = nullptr;

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
		};
		FORCE_INLINE void ProxySessionThread::run()
		{
			mParent->SessionRun();
		}
	}//namespace IPC
}//namespace X