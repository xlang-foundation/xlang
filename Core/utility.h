#pragma once
#include <vector>
#include <string>
#include <float.h>

bool RunProcess(std::string cmd,
	std::string initPath,
	bool newConsole,
	unsigned long& processId);

long long getCurMilliTimeStamp();
unsigned long GetPID();
unsigned long GetThreadID();
long long rand64();
double randDouble(double m0 = LDBL_MIN, double mx = LDBL_MAX);
std::vector<std::string> split(const std::string& str, char delim);
std::vector<std::string> split(const std::string& str, const char* delim);
std::string& rtrim(std::string& s);
std::string& ltrim(std::string& s);
std::string& trim(std::string& s);
std::string StringifyString(const std::string& str);
void ReplaceAll(std::string& data, std::string toSearch, std::string replaceStr);
std::string ExtName(std::string filePath);
bool exists(const std::string& name);
bool dir(std::string search_pat,
	std::vector<std::string>& subfolders,
	std::vector<std::string>& files);
bool file_search(std::string folder,
	std::string fileName,
	std::vector<std::string>& outFiles,bool findAll=false);
bool IsAbsPath(std::string& strPath);
bool LoadStringFromFile(std::string& fileName, std::string& content);
