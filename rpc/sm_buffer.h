#pragma once


#include "Locker.h"
#include "service_def.h"

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
        bool ClientConnect(unsigned long long key, int bufSize,
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
        inline int BufferSize()
        {
            return m_BufferSize - sizeof(PayloadFrameHead);
        }
        void ReleaseEvents();
        void Close();
    private:
        bool SendMsg(unsigned long long key);
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
        Locker mSharedMemLock;
        char* mShmPtr = 0;
        int m_BufferSize = 0;
        SwapBufferUser m_user = SwapBufferUser::Server;
        bool Wait(PasWaitHandle h, int timeoutMS);
    };
}
