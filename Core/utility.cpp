#include "utility.h"

#if (WIN32)
#include <Windows.h>
#else
#include <dirent.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#endif


long long getCurMilliTimeStamp()
{
#if (WIN32)
	return (long long)GetTickCount64();
#else
	struct timeval tv;
	gettimeofday(&tv, NULL);

	return tv.tv_sec * 1000 + tv.tv_usec / 1000;
#endif
}
unsigned long GetThreadID()
{
	unsigned long tid = 0;
#if (WIN32)
	tid = ::GetCurrentThreadId();
#else
#include <sys/types.h>
#include <unistd.h>
	tid = gettid();
#endif
	return tid;
}