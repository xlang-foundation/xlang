#pragma once
#include <vector>
#include <string>

bool RunProcess(std::string cmd,
	std::string initPath,
	bool newConsole,
	unsigned long& processId);

long long getCurMilliTimeStamp();
unsigned long GetThreadID();
long long rand64();
std::vector<std::string> split(const std::string& str, char delim);
std::vector<std::string> split(const std::string& str, const char* delim);
std::string& rtrim(std::string& s);
std::string& ltrim(std::string& s);
std::string& trim(std::string& s);