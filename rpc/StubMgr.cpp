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

#include "StubMgr.h"
#include "Stub.h"
#include "port.h"

namespace X
{
    void RemotingManager::Register(unsigned int callID,
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

    void RemotingManager::Unregister(unsigned int callID)
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
    void RemotingManager::CreateStub(unsigned long long key)
    {
        XLangStub* pStub = NULL;
        mStubLock.Lock();
        for (auto s : mStubs)
        {
            if (s->GetKey() == key)
            {
                pStub = s;
                break;
            }
        }
        mStubLock.Unlock();
        if (pStub == NULL)
        {
            pStub = new XLangStub();
            pStub->AddRef();//keep a reference for mStubs
            mStubLock.Lock();
            unsigned long long sid = ++mLastSessionID;
            pStub->SetSessionId(sid);
            mStubs.push_back(pStub);
            mStubLock.Unlock();
        }
        if (pStub->Create(key))
        {
            pStub->Start();
        }
    }
    bool RemotingManager::IsStubExist(unsigned long long sessionId)
    {
        bool bExist = false;
        mStubLock.Lock();
        for (auto it : mStubs)
        {
            if (it->GetSessionId() == sessionId)
            {
                bExist = true;
                break;
            }
        }
        mStubLock.Unlock();
        return bExist;
    }
    void RemotingManager::CloseStub(void* pStub)
    {
        XLangStub* pHostStub = (XLangStub*)pStub;
        if (pHostStub == nullptr)
        {
            return;
        }
        //check if this is only one reference to hold for this stub
        //because the reference in mStubs
        int cnt = 0;
        while ((cnt < 9999) && pHostStub->GetRefCount() > 1)
        {
            US_SLEEP(33000);
            cnt++;
        }

        pHostStub->Quit();

        pHostStub->Stop();
        bool bExist = false;
        mStubLock.Lock();
        for (auto it = mStubs.begin(); it != mStubs.end(); it++)
        {
            if (*it == pHostStub)
            {
                mStubs.erase(it);
                bExist = true;
                break;
            }
        }
        mStubLock.Unlock();
        if (bExist)
        {
			pHostStub->Release();
		}
    }
    void CallWorker::run()
    {
        while (m_bRun)
        {
            m_pWait->Wait(-1);
            m_bIdle = false;
            m_func();
            m_bIdle = true;
        }
    }
    CallWorker::CallWorker()
    {
        m_pWait = new XWait();
    }
    CallWorker::~CallWorker()
    {
        delete m_pWait;
    }
}