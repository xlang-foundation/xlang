#include "wait.h"
#if (WIN32)
#include <Windows.h>
#elif defined(__APPLE__1)
#include <dispatch/dispatch.h>
#else
#include <semaphore.h>
#include <sys/time.h>
#endif

XWait::XWait(bool autoReset) : m_autoReset(autoReset) {
#if defined(WIN32)
    HANDLE hEvt = CreateEvent(NULL, !autoReset, FALSE, NULL);
    m_waitHandle = hEvt;
#elif defined(__APPLE__1)
    dispatch_semaphore_t sem = dispatch_semaphore_create(0);
    m_waitHandle = sem;
#else
    sem_t* sem = new sem_t;
    if (sem_init(sem, 0, 0) != -1) {
        m_waitHandle = sem;
    }
    else {
        delete sem;
    }
#endif
}

XWait::~XWait() {
    if (m_waitHandle) {
#if defined(WIN32)
        HANDLE hEvt = (HANDLE)m_waitHandle;
        CloseHandle(hEvt);
#elif defined(__APPLE__1)
        dispatch_release((dispatch_semaphore_t)m_waitHandle);
#else
        sem_t* sem = (sem_t*)m_waitHandle;
        sem_destroy(sem);
        delete sem;
#endif
    }
}

bool XWait::Wait(int timeoutMS) {
    if (m_waitHandle == nullptr) {
        return false;
    }
#if defined(WIN32)
    return (WAIT_OBJECT_0 == WaitForSingleObject((HANDLE)m_waitHandle, timeoutMS));
#elif defined(__APPLE__1)
    dispatch_time_t timeout = timeoutMS == -1 ? DISPATCH_TIME_FOREVER :
        dispatch_time(DISPATCH_TIME_NOW, timeoutMS * NSEC_PER_MSEC);
    bool result = dispatch_semaphore_wait((dispatch_semaphore_t)m_waitHandle, timeout) == 0;
    if (result && m_autoReset) {
        dispatch_semaphore_signal((dispatch_semaphore_t)m_waitHandle);
    }
    return result;
#else
    struct timeval now;
    gettimeofday(&now, nullptr);
    struct timespec ts;
    ts.tv_sec = now.tv_sec + timeoutMS / 1000;
    ts.tv_nsec = now.tv_usec * 1000 + (timeoutMS % 1000) * 1000000;
    if (ts.tv_nsec >= 1000000000) {
        ts.tv_sec += ts.tv_nsec / 1000000000;
        ts.tv_nsec %= 1000000000;
    }
    int ret = sem_timedwait((sem_t*)m_waitHandle, &ts);
    if (ret == -1) {
        return false;
    }
    if (m_autoReset) {
        sem_post((sem_t*)m_waitHandle);
    }
    return true;
#endif
}

void XWait::Reset() {
#if defined(WIN32)
    ResetEvent((HANDLE)m_waitHandle);
#elif defined(__APPLE__1)
    // macOS does not support resetting dispatch semaphores directly.
    // This action is complex and usually not needed with dispatch semaphores.
    // Consider whether this functionality is necessary or if another pattern can be used.
#else
    sem_t* sem = (sem_t*)m_waitHandle;
    sem_init(sem, 0, 0);
#endif
}

void XWait::Release() {
    if (m_waitHandle) {
#if defined(WIN32)
        SetEvent((HANDLE)m_waitHandle);
#elif defined(__APPLE__1)
        dispatch_semaphore_signal((dispatch_semaphore_t)m_waitHandle);
#else
        sem_post((sem_t*)m_waitHandle);
#endif
    }
}
