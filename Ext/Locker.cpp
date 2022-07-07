#include "Locker.h"
#ifdef WIN32
#include <Windows.h>
#else
#include <mutex> 
#endif


Locker::Locker(void)
{
#if (WIN32)
	auto* cs = new CRITICAL_SECTION();
    InitializeCriticalSection(cs);
	m_cs = (void*)cs;
#else
	m_cs = (void*)new std::mutex();
#endif
}


Locker::~Locker(void)
{
#if (WIN32)
    DeleteCriticalSection((CRITICAL_SECTION*)m_cs);
	delete (CRITICAL_SECTION*)m_cs;
#else
	delete (std::mutex*)m_cs;
#endif

}
void Locker::Lock()
{
#ifndef WIN32
	m_cs->lock();
#else
	::EnterCriticalSection((CRITICAL_SECTION*)m_cs);
#endif
}

void Locker::Unlock()
{
#ifndef WIN32
	m_cs->unlock();
#else
	::LeaveCriticalSection((CRITICAL_SECTION*)m_cs);
#endif
}