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
#include <sstream>

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
std::vector<std::string> split(const std::string& str, char delim)
{
	std::vector<std::string> list;
	std::string temp;
	std::stringstream ss(str);
	while (std::getline(ss, temp, delim))
	{
		trim(temp);
		list.push_back(temp);
	}
	return list;
}
std::vector<std::string> split(const std::string& str, const char* delim)
{
	std::vector<std::string> list;
	std::string temp;
	size_t pos = 0;
	size_t pos2 = 0;
	size_t delim_size = strlen(delim);
	while ((pos2 = str.find(delim, pos)) != std::string::npos)
	{
		temp = str.substr(pos, pos2 - pos);
		trim(temp);
		list.push_back(temp);
		pos = pos2 + delim_size;
	}
	if (pos != std::string::npos)
	{
		temp = str.substr(pos);
		trim(temp);
		list.push_back(temp);
	}
	return list;
}
#define TRIMOFF_CHARS " \t\n\r\f\v"

// trim from end of string (right)
std::string& rtrim(std::string& s)
{
	s.erase(s.find_last_not_of(TRIMOFF_CHARS) + 1);
	return s;
}

// trim from beginning of string (left)
std::string& ltrim(std::string& s)
{
	s.erase(0, s.find_first_not_of(TRIMOFF_CHARS));
	return s;
}

// trim from both ends of string (right then left)
std::string& trim(std::string& s)
{
	return ltrim(rtrim(s));
}
