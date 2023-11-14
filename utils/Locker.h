#ifndef Locker_H
#define Locker_H

#ifdef WIN32
#include <Windows.h>
#else
#include <mutex> 
#endif

class Locker
{
	void* m_cs = nullptr;
	bool m_bEnable = true;
public:
	inline Locker(bool enable = true)
	{
		m_bEnable = enable;
		if (!enable)
		{
			return;
		}
#if (WIN32)
		auto* cs = new CRITICAL_SECTION();
		InitializeCriticalSection(cs);
		m_cs = (void*)cs;
#else
		m_cs = (void*)new std::mutex();
#endif
	}
	inline ~Locker()
	{
		if (!m_bEnable)
		{
			return;
		}
#if (WIN32)
		DeleteCriticalSection((CRITICAL_SECTION*)m_cs);
		delete (CRITICAL_SECTION*)m_cs;
#else
		delete (std::mutex*)m_cs;
#endif
	}

	inline void Lock()
	{
		if (!m_bEnable)
		{
			return;
		}
#if (WIN32)
		::EnterCriticalSection((CRITICAL_SECTION*)m_cs);
#else
		((std::mutex*)m_cs)->lock();
#endif
	}
	inline void Unlock()
	{
		if (!m_bEnable)
		{
			return;
		}
#if (WIN32)
		::LeaveCriticalSection((CRITICAL_SECTION*)m_cs);
#else
		((std::mutex*)m_cs)->unlock();
#endif
	}
};

class AutoLock
{
public:
	inline AutoLock(Locker& lk)
	{
		m_lock = &lk;
		lk.Lock();
	}

	inline ~AutoLock()
	{
		if (m_lock)
		{
			m_lock->Unlock();
		}
	}

private:
	Locker* m_lock;
};

#endif //Locker_H
