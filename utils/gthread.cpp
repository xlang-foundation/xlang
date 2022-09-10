#include "gthread.h"
#if (WIN32)
#include <Windows.h>
#else
#include <pthread.h>
#endif

#if (WIN32)
DWORD WINAPI gthread_proc(LPVOID param)
#else
static void* gthread_proc(void* param)
#endif
{
    GThread* pThis  =(GThread*)param;
    if(pThis)
    {
        pThis->run();
    }
    return 0;
}

bool GThread::Start()
{
    bool bOK = false;
#if (WIN32)
    DWORD dwTId;
    mThreadHandle = CreateThread(NULL, 0, gthread_proc, this, 0, &dwTId);
    if (mThreadHandle != NULL)
    {
        bOK = true;
    }
#else
    pthread_t pt;
    int rc = pthread_create(&pt, NULL, gthread_proc, (void *)this);
    if(rc ==0)
    {
        mThreadHandle = (void*)pt;
        bOK = true;
    }
#endif
    return bOK;
}
void GThread::Stop()
{
    WaitToEnd();
#if (WIN32)
    if (mThreadHandle)
    {
        CloseHandle(mThreadHandle);
        mThreadHandle = NULL;
    }
#endif
}
void GThread::WaitToEnd()
{
#if (WIN32)
    if (mThreadHandle)
    {
        ::WaitForSingleObject(mThreadHandle, INFINITE);
    }
#else
    void* status = nullptr;
    pthread_join((pthread_t)mThreadHandle, &status);
#endif
}
#if (WIN32)
DWORD WINAPI gthread_proc2(LPVOID param)
#else
static void* gthread_proc2(void* param)
#endif
{
    GThread2* pThis = (GThread2*)param;
    if (pThis)
    {
        pThis->run2();
    }
    return 0;
}
bool GThread2::Start()
{
    bool bOK = GThread::Start();
    if (!bOK)
    {
        return false;
    }
#if (WIN32)
    DWORD dwTId;
    mThreadHandle2 = CreateThread(NULL, 0, gthread_proc2, this, 0, &dwTId);
    if (mThreadHandle2 != NULL)
    {
        bOK = true;
    }
#else
    pthread_t pt;
    int rc = pthread_create(&pt, NULL, gthread_proc2, (void*)this);
    if (rc == 0)
    {
        mThreadHandle2 = (void*)pt;
        bOK = true;
    }
#endif
    return bOK;
}

void GThread2::Stop()
{
    GThread::Stop();
#if (WIN32)
    if (mThreadHandle2)
    {
        CloseHandle(mThreadHandle2);
        mThreadHandle2 = NULL;
    }
#endif
}
void GThread2::WaitToEnd()
{
    GThread::WaitToEnd();
#if (WIN32)
    if (mThreadHandle2)
    {
        ::WaitForSingleObject(mThreadHandle2, INFINITE);
    }
#else
    void* status = nullptr;
    pthread_join((pthread_t)mThreadHandle2, &status);
#endif
}
