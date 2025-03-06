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
        inline void RemotingMethod::Register(unsigned int callID,
            RemotingCallBase* pHandler,
            std::string funcName,
            std::vector<std::string> inputTypes,
            std::string retType)
        {
            RemoteFuncInfo* pFuncInfo = nullptr;
            mLockMapFuncs.Lock();
            auto it = mMapFuncs.find(callID);
            if (it != mMapFuncs.end())
            {
                pFuncInfo = it->second;
            }
            else
            {
                pFuncInfo = new RemoteFuncInfo();
                mMapFuncs.emplace(std::make_pair(callID, pFuncInfo));
            }
            pFuncInfo->callID = callID;
            pFuncInfo->pHandler = pHandler;
            pFuncInfo->funcName = funcName;
            pFuncInfo->inputTypes = inputTypes;
            pFuncInfo->retType = retType;
            mLockMapFuncs.Unlock();
        }

        inline void RemotingMethod::Unregister(unsigned int callID)
        {
            mLockMapFuncs.Lock();
            auto it = mMapFuncs.find(callID);
            if (it != mMapFuncs.end())
            {
                delete it->second;
                mMapFuncs.erase(it);
            }
            mLockMapFuncs.Unlock();
        }
	}
}