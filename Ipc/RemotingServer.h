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
            void Quit();
            void Close();
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
