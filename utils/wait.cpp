#include "wait.h"
#if (WIN32)
#include <Windows.h>
#else
#include <semaphore.h>
#include <sys/time.h>
#endif

#if defined(__APPLE__)
#include <pthread.h>
#include <semaphore.h>
#include <sys/time.h>
#include <errno.h>

int sem_timedwait(sem_t* sem, const struct timespec* abs_timeout) {
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

XWait::XWait(bool autoReset) : m_autoReset(autoReset) {
#if defined(WIN32)
    HANDLE hEvt = CreateEvent(NULL, !autoReset, FALSE, NULL);
    m_waitHandle = hEvt;
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
#else
    sem_t* sem = (sem_t*)m_waitHandle;
    sem_init(sem, 0, 0);
#endif
}

void XWait::Release() {
    if (m_waitHandle) {
#if defined(WIN32)
        SetEvent((HANDLE)m_waitHandle);
#else
        sem_post((sem_t*)m_waitHandle);
#endif
    }
}
