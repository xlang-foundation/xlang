#pragma once
#include "Singleton.h"
#include "Locker.h"
#include <vector>
#include "RemotingServer.h"

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
            //check if this is only one reference to hold for this stub
            //because the reference in mSrvs
            int cnt = 0;
            while ((cnt < 9999) && pRemotingSrv->RefCount() > 1)
            {
                US_SLEEP(33000);
                cnt++;
            }

            pRemotingSrv->Quit();

            pRemotingSrv->Stop();
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