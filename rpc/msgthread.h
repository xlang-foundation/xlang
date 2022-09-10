#ifndef MSGTHREAD_H
#define MSGTHREAD_H

#include "singleton.h"
#include "Locker.h"
#include <string>
#include "gthread.h"

namespace X 
{
    class MsgThread :
        public GThread,
        public Singleton<MsgThread>
    {
    public:
        MsgThread();
        ~MsgThread()
        {
        }
        void Stop();
        void SetNamespace(std::string& strNM)
        {
            m_namespace = strNM;
        }
        void run();
    private:
        void RemoveMsgId();
        bool mRun = true;
        Locker mMsgLock;
        int mMsgId = 0;
        std::string m_namespace;
    };
}
#endif // MSGTHREAD_H
