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
	m_cs = (void*)new std::recursive_mutex();
#endif
}


Locker::~Locker(void)
{
#if (WIN32)
    DeleteCriticalSection((CRITICAL_SECTION*)m_cs);
	delete (CRITICAL_SECTION*)m_cs;
#else
	delete (std::recursive_mutex*)m_cs;
#endif

}
void Locker::Lock()
{
#if (WIN32)
	::EnterCriticalSection((CRITICAL_SECTION*)m_cs);
#else
	((std::recursive_mutex*)m_cs)->lock();
#endif
}

void Locker::Unlock()
{
#if (WIN32)
	::LeaveCriticalSection((CRITICAL_SECTION*)m_cs);
#else
	((std::recursive_mutex*)m_cs)->unlock();
#endif
}