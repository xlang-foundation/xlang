#pragma once
#include <vector>
#include <string>
#include <float.h>

#if defined(BARE_METAL)
#include <climits>
#else
#include <limits.h>
#endif

bool RunProcess(std::string cmd,
	std::string initPath,
	bool newConsole,
	unsigned long& processId);

long long getCurMilliTimeStamp();
unsigned long GetPID();
unsigned long GetThreadID();
long long rand64(long long m0 = LLONG_MIN,long long mx = LLONG_MAX);
double randDouble(double m0 = LDBL_MIN, double mx = LDBL_MAX);
std::vector<std::string> split(const std::string& str, char delim,bool trimSpace =true);
std::vector<std::string> split(const std::string& str, const char* delim);
std::vector<std::string> splitWithChars(const std::string& str, const char* delim);
std::string concat(std::vector<std::string>& items, const char* delim);
std::string tostring(unsigned long long x);
std::string tostring(long x);
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
bool GetCurLibInfo(void* EntryFuncName,std::string& strFullPath, std::string& strFolderPath, std::string& strLibName);
const char* GetABIString(std::string& str);//return a string with ABI safety, this pointer will be released by g_pHost func
long long getCurMicroTimeStamp();
void _mkdir(const char* dir);