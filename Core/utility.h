#pragma once
#include <vector>
#include <string>

long long getCurMilliTimeStamp();
unsigned long GetThreadID();
long long rand64();
std::vector<std::string> split(const std::string& str, char delim);
std::vector<std::string> split(const std::string& str, const char* delim);
std::string& rtrim(std::string& s);
std::string& ltrim(std::string& s);
std::string& trim(std::string& s);