#include "utility.h"
#include "port.h"

#if (WIN32)
#include <Windows.h>
#else
#include <dirent.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <string.h>
#endif
#include <random>
#include <cmath>
#include <sstream>
#include <limits>
#include <fstream>


bool RunProcess(std::string cmd,
	std::string initPath, bool newConsole, unsigned long& processId)
{
#if (WIN32)
	STARTUPINFO StartupInfo;
	memset(&StartupInfo, 0, sizeof(STARTUPINFO));
	StartupInfo.cb = sizeof(STARTUPINFO);
	StartupInfo.wShowWindow = SW_SHOW;
	//StartupInfo.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
	//StartupInfo.hStdOutput = GetStdHandle(STD_OUTPUT_HANDLE);
	//StartupInfo.hStdError = GetStdHandle(STD_ERROR_HANDLE);

	PROCESS_INFORMATION ProcessInformation;
	memset(&ProcessInformation, 0, sizeof(PROCESS_INFORMATION));
	ProcessInformation.hThread = INVALID_HANDLE_VALUE;
	ProcessInformation.hProcess = INVALID_HANDLE_VALUE;

	BOOL bOK = CreateProcess(NULL, (LPSTR)cmd.c_str(), NULL, NULL, TRUE,
		newConsole ? CREATE_NEW_CONSOLE : 0,//if use CREATE_NEW_CONSOLE will show new cmd window
		NULL, NULL,
		&StartupInfo, &ProcessInformation);
	if (bOK)
	{
		processId = ProcessInformation.dwProcessId;
		//::WaitForSingleObject(ProcessInformation.hProcess, -1);
	}
	if (ProcessInformation.hProcess != INVALID_HANDLE_VALUE)
	{
		CloseHandle(ProcessInformation.hProcess);
	}
	if (ProcessInformation.hThread != INVALID_HANDLE_VALUE)
	{
		CloseHandle(ProcessInformation.hThread);
	}
#else
	pid_t child_pid;
	child_pid = fork();
	if (child_pid >= 0)
	{
		if (child_pid == 0)
		{//inside child process,run the executable 
			execlp(cmd.c_str(), initPath.c_str(), (char*)nullptr);
		}
		else
		{//inside parent process, child_pid is the id for child process
			processId = child_pid;
		}
	}
#endif
	return true;
}

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
unsigned long GetPID()
{
	unsigned long processId = 0;
#if (WIN32)
	processId = GetCurrentProcessId();
#else
	processId = getpid();
#endif
	return processId;
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
double randDouble(double m0, double mx)
{
	static std::random_device rd;
	static std::mt19937_64 e2(rd());
	static std::uniform_real_distribution<double> dist(m0, mx);
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
std::string StringifyString(const std::string& str)
{
	std::string str_out = "\"";

	std::string::const_iterator iter = str.begin();
	while (iter != str.end())
	{
		char chr = *iter;

		if (chr == '"' || chr == '\\' || chr == '/')
		{
			str_out += '\\';
			str_out += chr;
		}
		else if (chr == '\b')
		{
			str_out += "\\b";
		}
		else if (chr == '\f')
		{
			str_out += "\\f";
		}
		else if (chr == '\n')
		{
			str_out += "\\n";
		}
		else if (chr == '\r')
		{
			str_out += "\\r";
		}
		else if (chr == '\t')
		{
			str_out += "\\t";
		}
		else if (chr < ' ' || chr > 126)
		{
			str_out += "\\u";
			for (int i = 0; i < 4; i++)
			{
				int value = (chr >> 12) & 0xf;
				if (value >= 0 && value <= 9)
					str_out += (char)('0' + value);
				else if (value >= 10 && value <= 15)
					str_out += (char)('A' + (value - 10));
				chr <<= 4;
			}
		}
		else
		{
			str_out += chr;
		}

		iter++;
	}

	str_out += "\"";
	return str_out;
}
void ReplaceAll(std::string& data, std::string toSearch, std::string replaceStr)
{
	// Get the first occurrence
	size_t pos = data.find(toSearch);
	// Repeat till end is reached
	while (pos != std::string::npos)
	{
		// Replace this occurrence of Sub String
		data.replace(pos, toSearch.size(), replaceStr);
		// Get the next occurrence from the current position
		pos = data.find(toSearch, pos + replaceStr.size());
	}
}
std::string ExtName(std::string filePath)
{
	auto pos = filePath.rfind('.');
	if (pos != filePath.npos)
	{
		return filePath.substr(pos + 1);
	}
	else
	{
		return "";
	}
}
bool exists(const std::string& name)
{
	struct stat buffer;
	return (stat(name.c_str(), &buffer) == 0);
}
bool file_search(std::string folder,
	std::string fileName,
	std::vector<std::string>& outFiles,
	bool findAll)
{
	bool bFind = false;
	std::vector<std::string> subfolders;
	std::vector<std::string> files;
	bool bOK = dir(folder + Path_Sep_S + "*.*", subfolders, files);
	if (bOK)
	{
		for (auto& f : files)
		{
			if (f == fileName)
			{
				outFiles.push_back(folder + Path_Sep_S + f);
				bFind = true;
				break;
			}
		}
		for (auto& fd : subfolders)
		{
			bool bRet = file_search(folder + Path_Sep_S + fd, fileName, outFiles, findAll);
			if (bRet)
			{
				bFind = true;
				if (!findAll)
				{
					break;
				}
			}
		}
	}
	return bFind;
}
bool dir(std::string search_pat,
	std::vector<std::string>& subfolders,
	std::vector<std::string>& files)
{
	bool ret = false;
#if (WIN32)
	BOOL result = TRUE;
	WIN32_FIND_DATA ff;

	HANDLE findhandle = FindFirstFile(search_pat.c_str(), &ff);
	if (findhandle != INVALID_HANDLE_VALUE)
	{
		ret = true;
		BOOL res = TRUE;
		while (res)
		{
			// We only want directories
			if (ff.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			{
				std::string fileName(ff.cFileName);
				if (fileName != "." && fileName != "..")
				{
					subfolders.push_back(fileName);
				}
			}
			else
			{
				std::string fileName(ff.cFileName);
				files.push_back(fileName);
			}
			res = FindNextFile(findhandle, &ff);
		}
		FindClose(findhandle);
	}
#else
#include <dirent.h>
	DIR* dir;
	struct dirent* ent;
	if ((dir = opendir(search_pat.c_str())) != NULL)
	{
		ret = true;
		while ((ent = readdir(dir)) != NULL)
		{
			if (ent->d_type == DT_DIR)
			{
				std::string fileName(ent->d_name);
				if (fileName != "." && fileName != "..")
				{
					subfolders.push_back(fileName);
				}
			}
			else if (ent->d_type == DT_REG)//A regular file
			{
				std::string fileName(ent->d_name);
				files.push_back(fileName);
			}
		}
		closedir(dir);
	}
#endif
	return ret;
}
bool IsAbsPath(std::string& strPath)
{
	bool bIsAbs = false;
#if (WIN32)
	if (strPath.find(":/") != std::string::npos
		|| strPath.find(":\\") != std::string::npos
		|| strPath.find("\\\\") != std::string::npos//network path
		|| strPath.find("//") != std::string::npos//network path
		)
	{
		bIsAbs = true;
	}
#else
	if (strPath.find('/')==0)
	{
		bIsAbs = true;
	}
#endif
	return bIsAbs;
}
bool LoadStringFromFile(std::string& fileName, std::string& content)
{
	std::ifstream file(fileName);
	std::string data((std::istreambuf_iterator<char>(
		file)), std::istreambuf_iterator<char>());
	file.close();
	content = data;
	return true;
}