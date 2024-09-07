#pragma once
#include "singleton.h"
#include "IpcBase.h"
#include "Locker.h"

namespace X
{
	namespace IPC
	{
		class RemotingMethod :
			public Singleton<RemotingMethod>
		{
		public:
			void Register(unsigned int callID,
				RemotingCallBase* pHandler,
				std::string funcName,
				std::vector<std::string> inputTypes,
				std::string retType);
			void Unregister(unsigned int callID);


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
			Locker mLockMapFuncs;
			std::unordered_map<unsigned int, RemoteFuncInfo*> mMapFuncs;
		};
	}
}