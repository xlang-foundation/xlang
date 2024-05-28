#ifndef _XLangStub_H
#define _XLangStub_H

#include "gthread.h"
#include "service_def.h"
#include <vector>
#include "StubMgr.h"
#include "xproxy.h"

namespace X
{
    class SMSwapBuffer;
    class XLangStub;
    class StubWatch :
        public GThread
    {
    public:
        void SetParent(XLangStub* p)
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
        XLangStub* mParent = nullptr;
        unsigned long mPidToWatch = 0;
        void WaitToHostExit();
        bool mRun = true;
    };
    class XLangStub :
        public GThread,
        public RemotingProc
    {
        int mRefCount = 0;
        Locker m_lock;
    public:
        XLangStub();
        ~XLangStub();
        int GetRefCount()
        {
            m_lock.Lock();
            int ref =  mRefCount;
            m_lock.Unlock();
            return ref;
		}
        virtual int AddRef() override
        {
            m_lock.Lock();
            int ref = ++mRefCount;
            m_lock.Unlock();
            return ref;
        }
        virtual int Release() override
        {
            m_lock.Lock();
			int ref = --mRefCount;
			m_lock.Unlock();
            if (ref == 0)
            {
				delete this;
			}
			return 0;
		}
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
        //we don't support return value for this
        //only use for event handler from client side
        bool CallClient(ROBJ_ID clientObjId, ARGS& params, KWARGS& kwParams);
    protected:
        void run();
        void run2();
    private:
        StubWatch* m_pStubWatch;
        void ReceiveCall();
        unsigned long long mKey = 0;
        bool mRun = true;
        bool mInsideRecvCall1 = false;
        SMSwapBuffer* m_pSwapBuffer1 = nullptr;
        bool mInsideRecvCall2 = false;
        Locker m_locker_buffer2;
        SMSwapBuffer* m_pSwapBuffer2 = nullptr;
    private:
        void PostSessionCleanupCommand(unsigned long long sid);

        unsigned long long m_sessionId = 0;
        unsigned long m_clientPid = 0;

        bool ShakeHandsCall(void* pCallContext,SwapBufferStream& stream);
        // Inherited via RemotingProc
        virtual void NotifyBeforeCall(SwapBufferStream& stream) override;
        virtual void NotifyAfterCall(SwapBufferStream& stream, bool callIsOk) override;
        virtual void FinishCall(void* pCallContext,SwapBufferStream& stream,bool callIsOk) override;
    };
}
#endif // _XLangStub_H
