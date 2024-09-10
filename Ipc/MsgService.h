#ifndef MSGTHREAD_H
#define MSGTHREAD_H

#include "singleton.h"
#include "Locker.h"
#include <string>
#include "gthread.h"
#include "port.h"

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
            // Semaphore for additional process-level synchronization
            SEMAPHORE_HANDLE mSemaphore_For_Process= nullptr;
            bool MakeProcessLevelSemaphore();
            void Close();
            long mPort = 0;
            void RemoveMsgId();
            bool mRun = true;
            Locker mMsgLock;
            int mMsgId = 0;
        };
    } // namespace IPC
} // namespace X
#endif // MSGTHREAD_H
