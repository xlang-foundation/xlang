#pragma once
#include "SwapBufferStream.h"
#include "service_def.h"
#include "singleton.h"
#include <vector>
#include <unordered_map>
#include "Locker.h"
#include "gthread.h"
#include <functional>
#include "wait.h"

namespace X
{
	class XLangStub;
	class SMSwapBuffer;
	class RemotingProc
	{
	public:
		virtual void NotifyBeforeCall(SwapBufferStream& stream) = 0;
		virtual void NotifyAfterCall(SwapBufferStream& stream, bool callIsOk) = 0;
		virtual void FinishCall(void* pCallContext,SwapBufferStream& stream,bool callIsOk) = 0;
		virtual unsigned long long GetSessionId() = 0;
	};
	class RemotingCallBase
	{
	public:
		virtual bool Call(
			void* pCallContext, 
			unsigned int callType,
			SwapBufferStream& stream,
			RemotingProc* pProc) = 0;
	};
	struct RemoteFuncInfo
	{
		unsigned int callID;
		unsigned int _align;//For 8-bytes algined
		RemotingCallBase* pHandler;
		std::string funcName;
		std::vector<std::string> inputTypes;
		std::string retType;
	};
	using call_func = std::function<void()>;

	class CallWorker :
		public GThread
	{
		bool m_bRun = true;
		bool m_bIdle = false;
		XWait* m_pWait = nullptr;
		call_func m_func;
		// Inherited via GThread
		virtual void run() override;
	public:
		CallWorker();
		~CallWorker();
		inline bool IsIdle() {return m_bIdle;}
		inline void SetIdle(bool bIdle) { m_bIdle = bIdle; }
		void Call(call_func func)
		{
			m_func = func;
			m_pWait->Release();
		}
	};
	class RemotingManager :
		public Singleton<RemotingManager>
	{
	public:
		void Register(unsigned int callID,
			RemotingCallBase* pHandler,
			std::string funcName,
			std::vector<std::string> inputTypes,
			std::string retType);
		void Unregister(unsigned int callID);

		void CreateStub(unsigned long long key);
		void CloseStub(void* pStub);

		RemoteFuncInfo* Get(unsigned int callID)
		{
			RemoteFuncInfo* pFuncInfo = nullptr;
			mLockMapFuncs.Lock();
			auto it = mMapFuncs.find(callID);
			if (it != mMapFuncs.end())
			{
				pFuncInfo = it->second;
			}
			mLockMapFuncs.Unlock();
			return pFuncInfo;
		}
		CallWorker* GetIdleCallWorker()
		{
			CallWorker* pWorker = nullptr;
			mLockCallWorkers.Lock();
			for (auto it : mCallWorkers)
			{
				if (it->IsIdle())
				{
					pWorker = it;
					break;
				}
			}
			if (pWorker == nullptr)
			{
				pWorker = new CallWorker();
				pWorker->Start();
				mCallWorkers.push_back(pWorker);
			}
			pWorker->SetIdle(false);
			mLockCallWorkers.Unlock();
			return pWorker;
		}
	private:
		Locker mStubLock;
		unsigned long long mLastSessionID = 0;
		std::vector<XLangStub*> mStubs;
		Locker mLockCallWorkers;
		std::vector<CallWorker*> mCallWorkers;
		Locker mLockMapFuncs;
		std::unordered_map<unsigned int, RemoteFuncInfo*> mMapFuncs;
	};
}