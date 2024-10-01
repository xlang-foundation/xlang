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

#include "sm_buffer.h"
#include <string>
#include "utility.h"
#include "port.h"
#include <iostream>
#include "msgthread.h"

#if defined(__APPLE__)
#include <pthread.h>
#include <semaphore.h>
#include <sys/time.h>
#include <errno.h>

static int sem_timedwait(sem_t* sem, const struct timespec* abs_timeout) {
    pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER;
    pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
    int result = 0;

    pthread_mutex_lock(&mtx);
    while (sem_trywait(sem) != 0) {
        result = pthread_cond_timedwait(&cond, &mtx, abs_timeout);
        if (result == ETIMEDOUT) break;
    }
    pthread_mutex_unlock(&mtx);

    pthread_cond_destroy(&cond);
    pthread_mutex_destroy(&mtx);

    return result;
}

#endif

#if (WIN32)
#include <Windows.h>

#define RESETEVENT(evt)  ResetEvent(evt)
#define SETEVENT(evt)  SetEvent(evt)
#else
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/msg.h>
#include <fcntl.h>
#include <sys/time.h>

#define RESETEVENT(evt)
#define SETEVENT(evt)  sem_post(evt);
#endif

#define SLEEP() US_SLEEP(1)

#if (WIN32)
#include <Windows.h>
#include <sddl.h>
bool CheckIfAdmin() 
{
    BOOL isAdmin = FALSE;
    PSID adminGroup = NULL;

    // Create a SID for the administrators group
    SID_IDENTIFIER_AUTHORITY ntAuthority = SECURITY_NT_AUTHORITY;
    if (!AllocateAndInitializeSid(&ntAuthority, 2, SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_ADMINS, 0, 0, 0, 0, 0, 0, &adminGroup)) {
        return false;
    }

    // Check if the current token contains the admin SID
    if (!CheckTokenMembership(NULL, adminGroup, &isAdmin)) {
        isAdmin = FALSE;
    }

    FreeSid(adminGroup);
    return isAdmin;
}
#else
//in linux, we don't need to add global as prefix for IPC objects
//so we just return false
bool CheckIfAdmin()
{
	return false;
}
#endif

namespace X
{
#if (WIN32)
    BOOL ReadSlot(HANDLE hSlot, std::vector<X::pas_mesg_buffer>& msgs);
#endif
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
#elif __ANDROID__

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
#elif __ANDROID__

#else
            shmctl(mShmID, IPC_RMID, 0);
            mShmID = 0;
#endif
        }
        mSharedMemLock.Unlock();

        if (mSemaphore_For_Process)
        {
            CLOSE_SEMAPHORE(mSemaphore_For_Process);
			mSemaphore_For_Process = nullptr;
        }
    }
    
    bool SMSwapBuffer::HostCreate(unsigned long long key, int bufferSize)
    {
#if (WIN32)
        SECURITY_ATTRIBUTES sa;
        SECURITY_DESCRIPTOR sd;

        // Initialize the security descriptor
        if (!InitializeSecurityDescriptor(&sd, SECURITY_DESCRIPTOR_REVISION))
        {
            // Handle error
            return 1;
        }

        // Set the security descriptor DACL to a NULL DACL (grants access to everyone)
        if (!SetSecurityDescriptorDacl(&sd, TRUE, NULL, FALSE))
        {
            // Handle error
            return 1;
        }

        // Set the security attributes
        sa.nLength = sizeof(SECURITY_ATTRIBUTES);
        sa.lpSecurityDescriptor = &sd;
        sa.bInheritHandle = FALSE;

#endif
        bool IsAdmin = CheckIfAdmin();
        auto pid = GetPID();
        std::string semaphoreName =
            IsAdmin ? "Global\\XlangServerSemaphore_" : "XlangServerSemaphore_";
        semaphoreName += std::to_string(pid);
        mSemaphore_For_Process = CREATE_SEMAPHORE(sa,semaphoreName.c_str());
        if (mSemaphore_For_Process == nullptr)
        {
            std::cout << "Create semaphore "<< semaphoreName <<" failed"<<std::endl;
            return false;
        }
        else
        {
            std::cout << "Create semaphore " << semaphoreName << " OK" << std::endl;
		}
        m_user = SwapBufferUser::Server;

        const int Key_Len = 100;
        char szKey_s[Key_Len];
        SPRINTF(szKey_s, Key_Len, 
            IsAdmin ? "Global\\Galaxy_SM_Notify_Server_%llu": 
                    "Galaxy_SM_Notify_Server_%llu", key);
        char szKey_c[Key_Len];
        SPRINTF(szKey_c, Key_Len, 
            IsAdmin ? "Global\\Galaxy_SM_Notify_Client_%llu":
                    "Galaxy_SM_Notify_Client_%llu", key);

#if (WIN32)

        mNotiEvent_Server = CreateEvent(&sa, FALSE, FALSE, szKey_s);
        std::cout << "CreateEvent, Name:" << szKey_s << std::endl;
        mNotiEvent_Client = CreateEvent(&sa, FALSE, FALSE, szKey_c);
        std::cout << "CreateEvent, Name:" << szKey_c << std::endl;
        char mappingName[MAX_PATH];
        SPRINTF(mappingName, MAX_PATH, 
            IsAdmin ? "Global\\Galaxy_FileMappingObject_%llu":
                    "Galaxy_FileMappingObject_%llu", key);
        mShmID = CreateFileMapping(
            INVALID_HANDLE_VALUE,    // use paging file
            &sa,                    // default security
            PAGE_READWRITE,          // read/write access
            0,                       // maximum object size (high-order DWORD)
            bufferSize,  // maximum object size (low-order DWORD)
            mappingName);
        if (mShmID == nullptr)
        {
            DWORD error = GetLastError();
            std::cout << "CreateFileMapping:" << mappingName << " failed:"<< error << std::endl;
            return false;
        }
        else
        {
            std::cout << "CreateFileMapping:"<< mappingName <<" OK" << std::endl;
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
#elif __ANDROID__
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
    bool SMSwapBuffer::SendMsg(long port, unsigned long long shKey)
    {
        pas_mesg_buffer message;
        // msgget creates a message queue
        // and returns identifier
        message.mesg_type = (long)PAS_MSG_TYPE::CreateSharedMem;
		message.reserved = 0;
        message.shmKey = shKey;

#if (WIN32)
        std::string msgKey(PAS_MSG_KEY);
        if (port != 0)
        {
            msgKey += tostring(port);
        }
        HANDLE hFileMailSlot = CreateFile(msgKey.c_str(),
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
        if (fResult)
        {
        }
        CloseHandle(hFileMailSlot);
#elif __ANDROID__

#else
        // ftok to generate unique key
        key_t msgkey = port;
        printf("msgsnd with Key:0x%x\n", msgkey);
        int msgid = msgget(msgkey, 0666);
        //msgsnd to send message
        msgsnd(msgid, &message, sizeof(message), 0);
#endif
        return true;
    }
    bool SMSwapBuffer::ClientConnect(bool& usGlobal,long port, unsigned long long shKey,
        int bufSize,int timeoutMS, bool needSendMsg)
    {
        if (needSendMsg)
        {
            const int loopNum = 1000;
            int loopNo = 0;
            bool bSrvReady = false;
            while (loopNo < loopNum)
            {
                if (SendMsg(port, shKey))
                {
                    break;
                }
                MS_SLEEP(100);
                loopNo++;
            }
        }
        m_user = SwapBufferUser::Client;

        usGlobal = false;
        const int Key_Len = 100;
        char szKey_s[Key_Len];
        SPRINTF(szKey_s, Key_Len, "Global\\Galaxy_SM_Notify_Server_%llu", shKey);
        char szKey_c[Key_Len];
        SPRINTF(szKey_c, Key_Len, "Global\\Galaxy_SM_Notify_Client_%llu", shKey);

#if (WIN32)
        //Non-Global
        char szKey_s_l[Key_Len];
        SPRINTF(szKey_s_l, Key_Len, "Galaxy_SM_Notify_Server_%llu", shKey);
        char szKey_c_l[Key_Len];
        SPRINTF(szKey_c_l, Key_Len, "Galaxy_SM_Notify_Client_%llu", shKey);

        char mappingName[MAX_PATH];
        sprintf_s(mappingName, "Global\\Galaxy_FileMappingObject_%llu", shKey);

        char mappingName_l[MAX_PATH];
        sprintf_s(mappingName_l, "Galaxy_FileMappingObject_%llu", shKey);

        const int loopNum = 1000;
        int loopNo = 0;
        bool bSrvReady = false;
        while (mNotiEvent_Server == nullptr && loopNo < loopNum)
        {
            mNotiEvent_Server = OpenEvent(EVENT_ALL_ACCESS, FALSE,szKey_s);
            if (mNotiEvent_Server == nullptr)
            {
                mNotiEvent_Server = OpenEvent(EVENT_ALL_ACCESS, FALSE, szKey_s_l);
            }
            else
            {
                usGlobal = true;
            }
            if (mNotiEvent_Server != nullptr)
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
        loopNo = 0;
        bSrvReady = false;
        while (loopNo < loopNum)
        {
            mShmID = CreateFileMapping(
                INVALID_HANDLE_VALUE,
                NULL,
                PAGE_READWRITE,
                0,
                bufSize,
                usGlobal?mappingName: mappingName_l);
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

        loopNo = 0;
        bSrvReady = false;
        while (mNotiEvent_Client == nullptr && loopNo < loopNum)
        {
            mNotiEvent_Client = OpenEvent(EVENT_ALL_ACCESS, FALSE, 
                usGlobal?szKey_c: szKey_c_l);
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
#elif __ANDROID__

#else
        const int loopNum = 1000;
        int loopNo = 0;
        bool bSrvReady = false;
        int permission = 0666;
        while (loopNo < loopNum)
        {
            mShmID = shmget(shKey, (size_t)bufSize, permission);
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
            printf("shmget:%llu,Loop:%d\n", shKey, loopNo);
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
            printf("sem_open:%llu,Loop:%d\n", shKey, loopNo);
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
            printf("sem_open:0x%llu,Loop:%d\n", shKey, loopNo);
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
        //std::cout  << "ClientConnect:OK" << std::endl;
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
