#include "sm_buffer.h"
#include <string>
#include "utility.h"
#include "port.h"
#include <iostream>

#if (WIN32)
#include <Windows.h>

#define RESETEVENT(evt)  ResetEvent(evt)
#define SETEVENT(evt)  SetEvent(evt)
#else
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/msg.h>
#include <fcntl.h>

#define RESETEVENT(evt)
#define SETEVENT(evt)  sem_post(evt);
#endif

#define SLEEP() US_SLEEP(1)

namespace X
{
    SMSwapBuffer::SMSwapBuffer()
    {

    }

    SMSwapBuffer::~SMSwapBuffer()
    {
        Close();
    }

    bool SMSwapBuffer::Wait(PasWaitHandle h, int timeoutMS)
    {
#if (WIN32)
        return (WAIT_OBJECT_0 == ::WaitForSingleObject(h, timeoutMS));
#else
        struct timeval now;
        struct timespec ts;
        gettimeofday(&now, NULL);
        if (timeoutMS == -1)
        {
            now.tv_sec += 3600 * 24;
        }
        else
        {
            now.tv_usec += timeoutMS * 1000;
            if (now.tv_usec >= 1000000)
            {
                now.tv_sec += now.tv_usec / 1000000;
                now.tv_usec %= 1000000;
            }
        }

        ts.tv_sec = now.tv_sec;
        ts.tv_nsec = now.tv_usec * 1000;

        int ret = sem_timedwait((sem_t*)h, &ts);
        if (ret == -1)
        {
            return false;
        }
        else
        {
            return true;
        }
#endif
    }
    void SMSwapBuffer::ReleaseEvents()
    {
        if (mNotiEvent_Server)
        {
            SETEVENT(mNotiEvent_Server);
        }
        if (mNotiEvent_Client)
        {
            SETEVENT(mNotiEvent_Client);
        }
    }
    void SMSwapBuffer::Close()
    {
        mClosed = true;

        if (mNotiEvent_Server)
        {
#if (WIN32)
            SetEvent(mNotiEvent_Server);
            CloseHandle(mNotiEvent_Server);
#else
            sem_post(mNotiEvent_Server);
            sem_destroy(mNotiEvent_Server);
#endif
            mNotiEvent_Server = nullptr;
        }
        if (mNotiEvent_Client)
        {
#if (WIN32)
            SetEvent(mNotiEvent_Client);
            CloseHandle(mNotiEvent_Client);
#else
            sem_post(mNotiEvent_Client);
            sem_destroy(mNotiEvent_Client);
#endif
            mNotiEvent_Client = nullptr;
        }
        mSharedMemLock.Lock();
        if (mShmPtr)
        {
#if (WIN32)
            UnmapViewOfFile(mShmPtr);
#else
            shmdt(&mShmPtr);
#endif
            mShmPtr = nullptr;
        }
        if (mShmID)
        {
#if (WIN32)
            CloseHandle(mShmID);
            mShmID = nullptr;
#else
            shmctl(mShmID, IPC_RMID, 0);
            mShmID = 0;
#endif
        }
        mSharedMemLock.Unlock();
    }

    bool SMSwapBuffer::HostCreate(unsigned long long key, int bufferSize)
    {
        m_user = SwapBufferUser::Server;

        const int Key_Len = 100;
        char szKey_s[Key_Len];
        SPRINTF(szKey_s, Key_Len, "Galaxy_SM_Notify_Server_%llu", key);
        char szKey_c[Key_Len];
        SPRINTF(szKey_c, Key_Len, "Galaxy_SM_Notify_Client_%llu", key);
#if (WIN32)
        mNotiEvent_Server = CreateEvent(NULL, FALSE, FALSE, szKey_s);
        mNotiEvent_Client = CreateEvent(NULL, FALSE, FALSE, szKey_c);
        char mappingName[MAX_PATH];
        SPRINTF(mappingName, MAX_PATH, "Galaxy_FileMappingObject_%llu", key);
        mShmID = CreateFileMapping(
            INVALID_HANDLE_VALUE,    // use paging file
            NULL,                    // default security
            PAGE_READWRITE,          // read/write access
            0,                       // maximum object size (high-order DWORD)
            bufferSize,  // maximum object size (low-order DWORD)
            mappingName);
        if (mShmID == nullptr)
        {
            DWORD error = GetLastError();
            return false;
        }
        mShmPtr = (char*)MapViewOfFile(mShmID,   // handle to map object
            FILE_MAP_ALL_ACCESS, // read/write permission
            0,
            0,
            bufferSize);
        if (mShmPtr == nullptr)
        {
            DWORD error = GetLastError();
            return false;
        }
#else
        mShmID = shmget(key, (size_t)(bufferSize), IPC_CREAT | 0666);
        mShmPtr = (char*)shmat(mShmID, 0, 0);
        mNotiEvent_Server = sem_open(szKey_s, O_CREAT | O_EXCL, 0666, 0);
        mNotiEvent_Client = sem_open(szKey_c, O_CREAT | O_EXCL, 0666, 0);
#endif
        mClosed = false;
        m_BufferSize = bufferSize;
        return true;
    }
    bool SMSwapBuffer::SendMsg(unsigned long long key)
    {
        pas_mesg_buffer message;
        // msgget creates a message queue
        // and returns identifier
        message.mesg_type = (unsigned long long)PAS_MSG_TYPE::CreateSharedMem;
        message.shmKey = key;

#if (WIN32)
        HANDLE hFileMailSlot = CreateFile(PAS_MSG_KEY,
            GENERIC_WRITE,
            FILE_SHARE_READ,
            (LPSECURITY_ATTRIBUTES)NULL,
            OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL,
            (HANDLE)NULL);
        if (hFileMailSlot == INVALID_HANDLE_VALUE)
        {
            return false;
        }
        DWORD cbWritten;
        BOOL fResult = WriteFile(hFileMailSlot,
            &message,
            sizeof(message),
            &cbWritten,
            (LPOVERLAPPED)NULL);
        CloseHandle(hFileMailSlot);
#else
        // ftok to generate unique key
        key_t msgkey = PAS_MSG_KEY;
        printf("msgsnd with Key:0x%x\n", msgkey);
        int msgid = msgget(msgkey, 0666);
        //msgsnd to send message
        msgsnd(msgid, &message, sizeof(message), 0);
#endif
        return true;
    }
    bool SMSwapBuffer::ClientConnect(unsigned long long key, int bufSize,
        int timeoutMS, bool needSendMsg)
    {
        if (needSendMsg)
        {
            SendMsg(key);
        }
        m_user = SwapBufferUser::Client;

        const int Key_Len = 100;
        char szKey_s[Key_Len];
        SPRINTF(szKey_s, Key_Len, "Galaxy_SM_Notify_Server_%llu", key);
        char szKey_c[Key_Len];
        SPRINTF(szKey_c, Key_Len, "Galaxy_SM_Notify_Client_%llu", key);

#if (WIN32)
        char mappingName[MAX_PATH];
        sprintf_s(mappingName, "Galaxy_FileMappingObject_%llu", key);
        const int loopNum = 1000;
        int loopNo = 0;
        bool bSrvReady = false;
        while (loopNo < loopNum)
        {
            mShmID = CreateFileMapping(
                INVALID_HANDLE_VALUE,
                NULL,
                PAGE_READWRITE,
                0,
                bufSize,
                mappingName);
            if (mShmID != NULL)
            {
                mShmPtr = (char*)MapViewOfFile(mShmID, FILE_MAP_ALL_ACCESS,
                    0, 0, bufSize);
                bSrvReady = true;
                break;
            }
            MS_SLEEP(100);
            loopNo++;
        }
        if (!bSrvReady)
        {
            return false;
        }
        loopNo = 0;
        bSrvReady = false;
        while (mNotiEvent_Server == nullptr && loopNo < loopNum)
        {
            mNotiEvent_Server = OpenEvent(EVENT_ALL_ACCESS, FALSE, szKey_s);
            if (mNotiEvent_Server != nullptr)
            {
                bSrvReady = true;
                break;
            }
            MS_SLEEP(100);
            loopNo++;
        }
        loopNo = 0;
        bSrvReady = false;
        while (mNotiEvent_Client == nullptr && loopNo < loopNum)
        {
            mNotiEvent_Client = OpenEvent(EVENT_ALL_ACCESS, FALSE, szKey_c);
            if (mNotiEvent_Client != nullptr)
            {
                bSrvReady = true;
                break;
            }
            MS_SLEEP(100);
            loopNo++;
        }
        if (!bSrvReady)
        {
            return false;
        }
#else
        const int loopNum = 1000;
        int loopNo = 0;
        bool bSrvReady = false;
        int permission = 0666;
        while (loopNo < loopNum)
        {
            mShmID = shmget(key, (size_t)bufSize, permission);
            if (mShmID != -1)
            {
                mShmPtr = (char*)shmat(mShmID, NULL, 0);
                if (mShmPtr != nullptr)
                {
                    bSrvReady = true;
                    break;
                }
                else
                {//just in case
                    shmctl(mShmID, IPC_RMID, NULL);
                }
            }
            MS_SLEEP(100);
            loopNo++;
            printf("shmget:%llu,Loop:%d\n", key, loopNo);
        }
        if (!bSrvReady)
        {
            printf("shmget:failed\n");
            return false;
        }
        loopNo = 0;
        bSrvReady = false;
        while (mNotiEvent_Server == nullptr && loopNo < loopNum)
        {
            mNotiEvent_Server = sem_open(szKey_s, O_EXCL, permission, 0);
            if (mNotiEvent_Server != nullptr)
            {
                bSrvReady = true;
                break;
            }
            MS_SLEEP(100);
            loopNo++;
            printf("sem_open:%llu,Loop:%d\n", key, loopNo);
        }
        loopNo = 0;
        bSrvReady = false;
        while (mNotiEvent_Client == nullptr && loopNo < loopNum)
        {
            mNotiEvent_Client = sem_open(szKey_c, O_EXCL, permission, 0);
            if (mNotiEvent_Client != nullptr)
            {
                bSrvReady = true;
                break;
            }
            MS_SLEEP(100);
            loopNo++;
            printf("sem_open:0x%llu,Loop:%d\n", key, loopNo);
        }
        if (!bSrvReady)
        {
            printf("sem_open:failed\n");
            return false;
        }
#endif
        if (mShmPtr == NULL)
        {
            std::cout << "ClientConnect:failed"<<std::endl;
            return false;
        }
        std::cout  << "ClientConnect:OK" << std::endl;
        mClosed = false;
        m_BufferSize = bufSize;
        return true;
    }

    void SMSwapBuffer::BeginWrite()
    {
        RESETEVENT(mNotiEvent_Server);
        RESETEVENT(mNotiEvent_Client);
    }

    void SMSwapBuffer::EndWrite()
    {
        switch (m_user)
        {
        case SwapBufferUser::Server:
            SETEVENT(mNotiEvent_Client);
            break;
        case SwapBufferUser::Client:
            SETEVENT(mNotiEvent_Server);
            break;
        default:
            break;
        }
    }

    bool SMSwapBuffer::BeginRead(int timeoutMS)
    {
        bool bOK = false;
        switch (m_user)
        {
        case SwapBufferUser::Server:
            bOK = Wait(mNotiEvent_Server, timeoutMS);
            break;
        case SwapBufferUser::Client:
            bOK = Wait(mNotiEvent_Client, timeoutMS);
            break;
        default:
            break;
        }
        return bOK && (!mClosed);
    }

    void SMSwapBuffer::EndRead()
    {
    }

}