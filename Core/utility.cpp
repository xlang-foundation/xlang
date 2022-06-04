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
