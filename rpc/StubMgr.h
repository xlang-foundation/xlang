#pragma once
#include "SwapBufferStream.h"
#include "service_def.h"
#include "singleton.h"
#include <vector>
#include <unordered_map>
#include "Locker.h"

namespace X
{
	class XLangStub;
	class SMSwapBuffer;
	class RemotingProc
	{
	public:
		virtual void NotifyBeforeCall(int channel, SwapBufferStream& stream) = 0;
		virtual void NotifyAfterCall(int channel, SwapBufferStream& stream, bool callIsOk) = 0;
		virtual unsigned long long GetSessionId() = 0;
	};
	class RemotingCallBase
	{
	public:
		virtual bool Call(int channel,
			unsigned int callId,
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
	private:
		Locker mStubLock;
		unsigned long long mLastSessionID = 0;
		std::vector<XLangStub*> mStubs;

	private:
		Locker mLockMapFuncs;
		std::unordered_map<unsigned int, RemoteFuncInfo*> mMapFuncs;
	};
}