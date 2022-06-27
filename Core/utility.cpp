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
#include <random>
#include <cmath>


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

long long rand64()
{
	static std::random_device rd;
	static std::mt19937_64 e2(rd());
	static std::uniform_int_distribution<long long int> dist(std::llround(std::pow(2, 61)), std::llround(std::pow(2, 62)));
	return dist(e2);
}