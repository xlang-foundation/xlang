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
#include "Locker.h"
#include <vector>
#include "RemotingServer.h"
#include "port.h"

namespace X
{
	namespace IPC
	{
		class RemotingManager :
			public Singleton<RemotingManager>
		{
		private:
			Locker mSrvLock;
			unsigned long long mLastSessionID = 0;
			std::vector<RemotingServer*> mSrvs;
		public:
			void CreateServer(unsigned long long key);
			void CloseServer(void* pSrv);
			bool IsServerExist(unsigned long long sessionId);
		};
        inline void RemotingManager::CreateServer(unsigned long long key)
        {
            RemotingServer* pSrv = NULL;
            mSrvLock.Lock();
            for (auto s : mSrvs)
            {
                if (s->GetKey() == key)
                {
                    pSrv = s;
                    break;
                }
            }
            mSrvLock.Unlock();
            if (pSrv == NULL)
            {
                pSrv = new RemotingServer();
                pSrv->AddRef();//keep a reference for mSrvs
                mSrvLock.Lock();
                unsigned long long sid = ++mLastSessionID;
                pSrv->SetSessionId(sid);
                mSrvs.push_back(pSrv);
                mSrvLock.Unlock();
            }
            if (pSrv->Create(key))
            {
                pSrv->Start();
            }
        }
        inline bool RemotingManager::IsServerExist(unsigned long long sessionId)
        {
            bool bExist = false;
            mSrvLock.Lock();
            for (auto it : mSrvs)
            {
                if (it->GetSessionId() == sessionId)
                {
                    bExist = true;
                    break;
                }
            }
            mSrvLock.Unlock();
            return bExist;
        }
        inline void RemotingManager::CloseServer(void* pSrv)
        {
            RemotingServer* pRemotingSrv = (RemotingServer*)pSrv;
            if (pSrv == nullptr)
            {
                return;
            }
            pRemotingSrv->Quit();

            //check if this is only one reference to hold for this stub
            //because the reference in mSrvs
            int cnt = 0;
            while ((cnt < 9999) && pRemotingSrv->RefCount() > 1)
            {
                US_SLEEP(33000);
                cnt++;
            }

            bool bExist = false;
            mSrvLock.Lock();
            for (auto it = mSrvs.begin(); it != mSrvs.end(); it++)
            {
                if (*it == pSrv)
                {
                    mSrvs.erase(it);
                    bExist = true;
                    break;
                }
            }
            mSrvLock.Unlock();
            if (bExist)
            {
                pRemotingSrv->Release();
            }
        }
	}
}