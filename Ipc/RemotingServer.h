#ifndef _XLangStub_H
#define _XLangStub_H

#include "gthread.h"
#include "service_def.h"
#include <vector>
#include "Call.h"

namespace X
{
    namespace IPC 
    {

    class SMSwapBuffer;
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
        public GThread,
        public CallHandler
    {
        Locker m_lock;
    public:
        RemotingServer();
        ~RemotingServer();
        void SetSessionId(unsigned long long sid)
        {
            m_sessionId = sid;
        }
        bool Create(unsigned long long shmKey);
        void Quit();
        unsigned long long GetKey()
        {
            return mKey;
        }
        void WatchClientProcess(unsigned long pid);
        void NotifyFromWatch(bool processEnded);
        virtual unsigned long long GetSessionId() override
        {
            return m_sessionId;
        }
        void CleanupResourcesForSession(unsigned long long sid);
        unsigned long GetClientPid()
        {
            return m_clientPid;
        }
    protected:
        void run();
    private:
        StubWatch* m_pStubWatch;
        unsigned long long mKey = 0;
        bool mRun = true;
        bool mInsideRecvCall1 = false;
        bool mInsideRecvCall2 = false;

    private:
        void PostSessionCleanupCommand(unsigned long long sid);

        unsigned long long m_sessionId = 0;
        unsigned long m_clientPid = 0;

        void ShakeHandsCall(void* pCallContext,SwapBufferStream& stream) override;
    };

    }//namespace IPC
}//namespace X
#endif // _XLangStub_H
