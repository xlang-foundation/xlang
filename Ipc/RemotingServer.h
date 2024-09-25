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

#ifndef _XLangStub_H
#define _XLangStub_H

#include "service_def.h"
#include <vector>
#include "Call.h"

namespace X
{
    namespace IPC
    {
        class RemotingServer;
        class StubWatch :
            public GThread
        {
        public:
            void SetParent(RemotingServer* p)
            {
                mParent = p;
            }
            void SetPid(unsigned long pid)
            {
                mPidToWatch = pid;
            }
        protected:
            void run();
        private:
            RemotingServer* mParent = nullptr;
            unsigned long mPidToWatch = 0;
            void WaitToHostExit();
            bool mRun = true;
        };

        class RemotingServer : 
            public CallHandler
        {
        public:
            RemotingServer();
            ~RemotingServer();
            void SetSessionId(unsigned long long sid)
            {
                m_sessionId = sid;
            }
            bool Create(unsigned long long shmKey);
            unsigned long long GetKey()
            {
                return mKey;
            }

            virtual unsigned long long GetSessionId() override
            {
                return m_sessionId;
            }
            unsigned long GetClientPid()
            {
                return m_clientPid;
            }
            void OnClientExit();
        private:
            StubWatch* m_pStubWatch;
            unsigned long long mKey = 0;
            unsigned long long m_sessionId = 0;
            unsigned long m_clientPid = 0;
            void ShakeHandsCall(void* pCallContext, SwapBufferStream& stream) override;

            // Inherited via CallHandler
            void SetRootObjectName(const char* name) override;
            ROBJ_ID QueryRootObject(std::string& name) override;
            void SetTimeout(int timeout) override;
        };

    } // namespace IPC
} // namespace X

#endif // _XLangStub_H
