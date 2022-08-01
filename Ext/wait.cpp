#include "wait.h"
#if (WIN32)
#include <Windows.h>
#else
#include <semaphore.h>
#include <sys/time.h>
#endif
XWait::XWait(bool autoReset)
{
	m_autoReset = autoReset;
	XWaitHandle handle = nullptr;
#if (WIN32)
	HANDLE hEvt = CreateEvent(NULL, autoReset?FALSE:TRUE, FALSE, NULL);
	handle = hEvt;
#else
	sem_t* pWait = new sem_t;
	if (sem_init(pWait, 0, 0) != -1)
	{
		handle = pWait;
	}
	else
	{
		delete pWait;
	}
#endif
	m_waitHandle = handle;
}

XWait::~XWait()
{
	if (m_waitHandle)
	{
#if (WIN32)
		HANDLE hEvt = (HANDLE)m_waitHandle;
		CloseHandle(hEvt);
#else
		sem_t* pWait = (sem_t*)m_waitHandle;
		delete pWait;
#endif
	}
}

bool XWait::Wait(int timeoutMS)
{
	if (m_waitHandle == nullptr)
	{
		return false;
	}
#if (WIN32)
	return (WAIT_OBJECT_0 == ::WaitForSingleObject(
		(HANDLE)m_waitHandle, timeoutMS));
#else
	struct timeval now;
	struct timespec ts;
	gettimeofday(&now, nullptr);
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

	int ret = sem_timedwait((sem_t*)m_waitHandle, &ts);
	if (ret == -1)
	{
		return false;
	}
	else
	{
		if (m_autoReset)
		{
			sem_post((sem_t*)m_waitHandle);
		}
		return true;
	}
#endif
}

void XWait::Release()
{
	if (m_waitHandle)
	{
#if (WIN32)
		SetEvent((HANDLE)m_waitHandle);
#else
		sem_post((sem_t*)m_waitHandle);
#endif
	}
}
