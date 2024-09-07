#ifndef MSGTHREAD_H
#define MSGTHREAD_H

#include "singleton.h"
#include "Locker.h"
#include <string>
#include "gthread.h"

namespace X 
{
    namespace IPC
    {
        class MsgService :
            public GThread,
            public Singleton<MsgService>
        {
        public:
            MsgService();
            ~MsgService()
            {
            }
            void Stop();
            void run();
            void SetPort(long port)
            {
                mPort = port;
            }
        private:
            long mPort = 0;
            void RemoveMsgId();
            bool mRun = true;
            Locker mMsgLock;
            int mMsgId = 0;
        };
    } // namespace IPC
} // namespace X
#endif // MSGTHREAD_H
