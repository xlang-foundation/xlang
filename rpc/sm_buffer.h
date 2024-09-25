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

#pragma once



#include "Locker.h"
#include "service_def.h"
#include "port.h"

#if (WIN32)
#include <Windows.h>
#else
#include <semaphore.h>
#endif

namespace X
{
    typedef void* PasWaitHandle;
    enum class SwapBufferUser
    {
        Server,//read and write result
        Client//write and read result 
    };
    class SMSwapBuffer
    {
    public:
        SMSwapBuffer();
        virtual ~SMSwapBuffer();

        bool HostCreate(unsigned long long key, int bufSize);
        bool ClientConnect(bool& usGlobal,long port, unsigned long long shKey,int bufSize,
            int timeoutMS, bool needSendMsg = true);

        void BeginWrite();
        void EndWrite();
        bool BeginRead(int timeoutMS = -1);
        void EndRead();

        PayloadFrameHead& GetHead()
        {
            return *(PayloadFrameHead*)mShmPtr;
        }
        char* GetBuffer()
        {
            return mShmPtr + sizeof(PayloadFrameHead);
        }
        FORCE_INLINE int BufferSize()
        {
            return m_BufferSize - sizeof(PayloadFrameHead);
        }
        void ReleaseEvents();
        void Close();
    private:
        bool SendMsg(long port,unsigned long long shKey);
        bool mClosed = false;
#if (WIN32)
        HANDLE mShmID = 0;
        HANDLE mNotiEvent_Server = nullptr;
        HANDLE mNotiEvent_Client = nullptr;
#else
        int mShmID = 0;
        sem_t* mNotiEvent_Server = nullptr;
        sem_t* mNotiEvent_Client = nullptr;
#endif
        SEMAPHORE_HANDLE mSemaphore_For_Process = nullptr;
        Locker mSharedMemLock;
        char* mShmPtr = 0;
        int m_BufferSize = 0;
        SwapBufferUser m_user = SwapBufferUser::Server;
        bool Wait(PasWaitHandle h, int timeoutMS);
    };
}
