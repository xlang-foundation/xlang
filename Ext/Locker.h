#ifndef Locker_H
#define Locker_H

#ifdef WIN32
#include <Windows.h>
#else
#include <mutex> 
#endif
class Locker
{
public:
	Locker();
	~Locker();

	inline void Lock()
	{
#ifndef WIN32
		m_cs.lock();
#else
		::EnterCriticalSection(&m_cs);
#endif
	}

	inline void Unlock()
	{
#ifndef WIN32
		m_cs.unlock();
#else
		::LeaveCriticalSection(&m_cs);
#endif
	}

private:
#ifndef WIN32
	std::mutex m_cs;
#else
	CRITICAL_SECTION m_cs;
#endif
};

class AutoLock
{
public:
	AutoLock() :
		m_lock(NULL)
	{
	}

	AutoLock(Locker& lk)
	{
		m_lock = &lk;
		lk.Lock();
	}

	~AutoLock()
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
