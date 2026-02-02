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
