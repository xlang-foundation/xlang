#include "Locker.h"


Locker::Locker(void)
{
#if (WIN32)
    InitializeCriticalSection(&m_cs);
#endif
}


Locker::~Locker(void)
{
#if (WIN32)
    DeleteCriticalSection(&m_cs);
#endif
}
