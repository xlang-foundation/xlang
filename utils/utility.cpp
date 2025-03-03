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

#include "utility.h"
#include "port.h"

#if defined(WIN32)
#include <Windows.h>
#elif defined(BARE_METAL)
#include <cstring>
#include <cstdlib>
#else
#include <dirent.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <cstring>
#endif
#include <random>
#include <cmath>
#include <sstream>
#include <limits>
#include <fstream>
#include <chrono>

#if defined(__APPLE__)
#include <mach/mach.h>
#endif

bool RunProcess(std::string cmd, std::string initPath, bool newConsole, unsigned long& processId) {
#if defined(WIN32)
    STARTUPINFO StartupInfo;
    memset(&StartupInfo, 0, sizeof(STARTUPINFO));
    StartupInfo.cb = sizeof(STARTUPINFO);
    StartupInfo.wShowWindow = SW_SHOW;

    PROCESS_INFORMATION ProcessInformation;
    memset(&ProcessInformation, 0, sizeof(PROCESS_INFORMATION));
    ProcessInformation.hThread = INVALID_HANDLE_VALUE;
    ProcessInformation.hProcess = INVALID_HANDLE_VALUE;

    BOOL bOK = CreateProcess(NULL, (LPSTR)cmd.c_str(), NULL, NULL, TRUE,
        newConsole ? CREATE_NEW_CONSOLE : 0,
        NULL, NULL,
        &StartupInfo, &ProcessInformation);
    if (bOK) {
        processId = ProcessInformation.dwProcessId;
    }
    if (ProcessInformation.hProcess != INVALID_HANDLE_VALUE) {
        CloseHandle(ProcessInformation.hProcess);
    }
    if (ProcessInformation.hThread != INVALID_HANDLE_VALUE) {
        CloseHandle(ProcessInformation.hThread);
    }
    return bOK;
#elif defined(BARE_METAL)
    // Bare-metal implementation not possible, return false
    return false;
#else
    pid_t child_pid;
    child_pid = fork();
    if (child_pid >= 0) {
        if (child_pid == 0) {
            execlp(cmd.c_str(), initPath.c_str(), (char*)nullptr);
        }
        else {
            processId = child_pid;
        }
    }
    return child_pid >= 0;
#endif
}

long long getCurMilliTimeStamp() {
#if defined(WIN32)
    return (long long)GetTickCount64();
#elif defined(BARE_METAL)
    // Bare-metal implementation not possible, return 0
    return 0;
#else
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec * 1000 + tv.tv_usec / 1000;
#endif
}

unsigned long GetPID() {
#if defined(WIN32)
    return GetCurrentProcessId();
#elif defined(BARE_METAL)
    // Bare-metal implementation not possible, return 0
    return 0;
#else
    return getpid();
#endif
}

unsigned long GetThreadID() {
#if defined(WIN32)
    return ::GetCurrentThreadId();
#elif defined(__APPLE__)
    unsigned long tid = (unsigned long)mach_thread_self();
    mach_port_deallocate(mach_task_self(), tid);
    return tid;
#elif defined(BARE_METAL)
    // Bare-metal implementation not possible, return 0
    return 0;
#else
#if __GLIBC__ == 2 && __GLIBC_MINOR__ < 30
#include <sys/syscall.h>
#define gettid() syscall(SYS_gettid)
#else
#include <sys/types.h>
#include <unistd.h>
#endif
    return gettid();
#endif
}

long long rand64(long long m0, long long mx) {
#if defined(BARE_METAL)
    return m0 + (std::rand() % (mx - m0 + 1));
#else
    static std::random_device rd;
    static std::mt19937_64 e2(rd());
    static std::uniform_int_distribution<long long int> dist(m0, mx);
    return dist(e2);
#endif
}

double randDouble(double m0, double mx) {
#if defined(BARE_METAL)
    return m0 + (static_cast<double>(std::rand()) / RAND_MAX) * (mx - m0);
#else
    static std::random_device rd;
    static std::mt19937_64 e2(rd());
    static std::uniform_real_distribution<double> dist(m0, mx);
    return dist(e2);
#endif
}

std::string tostring(unsigned long long x) {
    const int buf_len = 100;
    char strBuf[buf_len];
    snprintf(strBuf, buf_len, "%llu", x);
    return strBuf;
}

std::string tostring(long x) {
    const int buf_len = 100;
    char strBuf[buf_len];
    snprintf(strBuf, buf_len, "%ld", x);
    return strBuf;
}

std::string concat(std::vector<std::string>& items, const char* delim) {
    if (items.empty()) {
        return "";
    }
    std::string strDelim(delim);
    std::string all = items[0];
    for (size_t i = 1; i < items.size(); i++) {
        all += strDelim + items[i];
    }
    return all;
}

std::vector<std::string> split(const std::string& str, char delim, bool trimSpace) {
    std::vector<std::string> list;
    std::string temp;
    std::stringstream ss(str);
    while (std::getline(ss, temp, delim)) {
        if (trimSpace) trim(temp);
        list.push_back(temp);
    }
    return list;
}

std::vector<std::string> splitWithChars(const std::string& str, const char* delim)
{
    std::vector<std::string> list;
    std::size_t prev = 0, pos;
    while ((pos = str.find_first_of(delim, prev)) != std::string::npos)
    {
        if (pos > prev)
        {
            list.push_back(str.substr(prev, pos - prev));
        }
        prev = pos + 1;
    }
    if (prev != std::string::npos)
    {
        list.push_back(str.substr(prev));
    }
    return list;
}

std::vector<std::string> split(const std::string& str, const char* delim) {
    std::vector<std::string> list;
    std::string temp;
    size_t pos = 0;
    size_t pos2 = 0;
    size_t delim_size = strlen(delim);
    while ((pos2 = str.find(delim, pos)) != std::string::npos) {
        temp = str.substr(pos, pos2 - pos);
        trim(temp);
        list.push_back(temp);
        pos = pos2 + delim_size;
    }
    if (pos != std::string::npos) {
        temp = str.substr(pos);
        trim(temp);
        list.push_back(temp);
    }
    return list;
}

#define TRIMOFF_CHARS " \t\n\r\f\v"

// trim from end of string (right)
std::string& rtrim(std::string& s) {
    s.erase(s.find_last_not_of(TRIMOFF_CHARS) + 1);
    return s;
}

// trim from beginning of string (left)
std::string& ltrim(std::string& s) {
    s.erase(0, s.find_first_not_of(TRIMOFF_CHARS));
    return s;
}

// trim from both ends of string (right then left)
std::string& trim(std::string& s) {
    return ltrim(rtrim(s));
}

std::string StringifyString(const std::string& str) {
    std::string str_out = "\"";
    std::string::const_iterator iter = str.begin();
    while (iter != str.end()) {
        char chr = *iter;
        if (chr == '"' || chr == '\\') {
            str_out += '\\';
            str_out += chr;
        }
        else if (chr == '\b') {
            str_out += "\\b";
        }
        else if (chr == '\f') {
            str_out += "\\f";
        }
        else if (chr == '\n') {
            str_out += "\\n";
        }
        else if (chr == '\r') {
            str_out += "\\r";
        }
        else if (chr == '\t') {
            str_out += "\\t";
        }
#if 0 // this part can't handle multi-byte UTF-8 sequences
        else if (chr < ' ' || chr > 126) {
            str_out += "\\u";
            for (int i = 0; i < 4; i++) {
                int value = (chr >> 12) & 0xf;
                if (value >= 0 && value <= 9)
                    str_out += (char)('0' + value);
                else if (value >= 10 && value <= 15)
                    str_out += (char)('A' + (value - 10));
                chr <<= 4;
            }
        }
#endif
        else {
            str_out += chr;
        }
        iter++;
    }
    str_out += "\"";
    return str_out;
}

void ReplaceAll(std::string& data, std::string toSearch, std::string replaceStr) {
    size_t pos = data.find(toSearch);
    while (pos != std::string::npos) {
        data.replace(pos, toSearch.size(), replaceStr);
        pos = data.find(toSearch, pos + replaceStr.size());
    }
}

std::string ExtName(std::string filePath) {
    auto pos = filePath.rfind('.');
    if (pos != filePath.npos) {
        return filePath.substr(pos + 1);
    }
    else {
        return "";
    }
}

bool exists(const std::string& name) {
#if defined(BARE_METAL)
    // Bare-metal implementation not possible, return false
    return false;
#else
    struct stat buffer;
    return (stat(name.c_str(), &buffer) == 0);
#endif
}

bool file_search(std::string folder, std::string fileName, std::vector<std::string>& outFiles, bool findAll) {
    bool bFind = false;
    std::vector<std::string> subfolders;
    std::vector<std::string> files;
    bool bOK = dir(folder, subfolders, files);
    if (bOK) {
        for (auto& f : files) {
            if (f == fileName) {
                outFiles.push_back(folder + Path_Sep_S + f);
                bFind = true;
                if (!findAll) {
                    break;
                }
            }
        }
        for (auto& fd : subfolders) {
            bool bRet = file_search(folder + Path_Sep_S + fd, fileName, outFiles, findAll);
            if (bRet) {
                bFind = true;
                if (!findAll) {
                    break;
                }
            }
        }
    }
    return bFind;
}

bool dir(std::string search_pat, std::vector<std::string>& subfolders, std::vector<std::string>& files) {
#if defined(WIN32)
    BOOL result = TRUE;
    WIN32_FIND_DATA ff;
    search_pat += "\\*.*";
    HANDLE findhandle = FindFirstFile(search_pat.c_str(), &ff);
    if (findhandle != INVALID_HANDLE_VALUE) {
        BOOL res = TRUE;
        while (res) {
            if (ff.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
                std::string fileName(ff.cFileName);
                if (fileName != "." && fileName != "..") {
                    subfolders.push_back(fileName);
                }
            }
            else {
                std::string fileName(ff.cFileName);
                files.push_back(fileName);
            }
            res = FindNextFile(findhandle, &ff);
        }
        FindClose(findhandle);
        return true;
    }
    return false;
#elif defined(BARE_METAL)
    // Bare-metal implementation not possible, return false
    return false;
#else
    DIR* dir;
    struct dirent* ent;
    if ((dir = opendir(search_pat.c_str())) != NULL) {
        while ((ent = readdir(dir)) != NULL) {
            if (ent->d_type == DT_DIR) {
                std::string fileName(ent->d_name);
                if (fileName != "." && fileName != "..") {
                    subfolders.push_back(fileName);
                }
            }
            else if (ent->d_type == DT_REG) {
                std::string fileName(ent->d_name);
                files.push_back(fileName);
            }
        }
        closedir(dir);
        return true;
    }
    return false;
#endif
}

bool IsAbsPath(std::string& strPath) {
#if defined(WIN32)
    return (strPath.find(":/") != std::string::npos ||
        strPath.find(":\\") != std::string::npos ||
        strPath.find("\\\\") != std::string::npos ||
        strPath.find("//") != std::string::npos);
#else
    return (strPath.find('/') == 0);
#endif
}

bool LoadStringFromFile(std::string& fileName, std::string& content) {
#if defined(BARE_METAL)
    // Bare-metal implementation not possible, return false
    return false;
#else
    std::ifstream file(fileName);
    std::string data((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    file.close();
    content = data;
    return true;
#endif
}

bool GetCurLibInfo(void* EntryFuncName, std::string& strFullPath, std::string& strFolderPath, std::string& strLibName) {
#if defined(WIN32)
    HMODULE hModule = NULL;
    GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS, (LPCTSTR)EntryFuncName, &hModule);
    char path[MAX_PATH];
    GetModuleFileName(hModule, path, MAX_PATH);
    std::string strPath(path);
    strFullPath = strPath;
    auto pos = strPath.rfind("\\");
    if (pos != std::string::npos) {
        strFolderPath = strPath.substr(0, pos);
        strLibName = strPath.substr(pos + 1);
    }
#elif defined(BARE_METAL)
    // Bare-metal implementation not possible, return false
    return false;
#else
    Dl_info dl_info;
    dladdr((void*)EntryFuncName, &dl_info);
    std::string strPath = dl_info.dli_fname;
    strFullPath = strPath;
    auto pos = strPath.rfind("/");
    if (pos != std::string::npos) {
        strFolderPath = strPath.substr(0, pos);
        strLibName = strPath.substr(pos + 1);
    }
#endif
    auto pos0 = strLibName.rfind(".");
    if (pos0 != std::string::npos) {
        strLibName = strLibName.substr(0, pos0);
    }
    return true;
}

const char* GetABIString(std::string& str) {
    char* retStr = new char[str.size() + 1];
    memcpy(retStr, str.data(), str.size() + 1);
    return retStr;
}

long long getCurMicroTimeStamp() {
#if defined(BARE_METAL)
    // Bare-metal implementation not possible, return 0
    return 0;
#else
    auto current_time = std::chrono::high_resolution_clock::now();
    long long current_time_ll = std::chrono::time_point_cast<std::chrono::microseconds>(current_time).time_since_epoch().count();
    return current_time_ll;
#endif
}

void _mkdir(const char* dir) {
#if defined(WIN32)
    CreateDirectory(dir, NULL);
#elif defined(BARE_METAL)
    // Bare-metal implementation not possible, do nothing
#else
    int state = access(dir, R_OK | W_OK);
    if (state != 0) {
        mkdir(dir, S_IRWXU);
    }
#endif
}

#ifdef _WIN32
std::string systemCpToUtf8(const std::string& input) {
	if (input.empty()) return "";
	int wideLen = MultiByteToWideChar(CP_ACP, 0, input.c_str(), -1, NULL, 0);
	if (wideLen == 0) return "";
	std::vector<wchar_t> wideStr(wideLen);
	if (MultiByteToWideChar(CP_ACP, 0, input.c_str(), -1, wideStr.data(), wideLen) == 0) {
		return "";
	}
	int utf8Len = WideCharToMultiByte(CP_UTF8, 0, wideStr.data(), -1, NULL, 0, NULL, NULL);
	if (utf8Len == 0) return "";
	std::vector<char> utf8Str(utf8Len);
	if (WideCharToMultiByte(CP_UTF8, 0, wideStr.data(), -1, utf8Str.data(), utf8Len, NULL, NULL) == 0) {
		return "";
	}
	return std::string(utf8Str.data());
}

std::string utf8ToSystemCp(const std::string& utf8Str) {
	if (utf8Str.empty()) return "";
	int wideLen = MultiByteToWideChar(CP_UTF8, 0, utf8Str.c_str(), -1, NULL, 0);
	if (wideLen == 0) return "";
	std::vector<wchar_t> wideStr(wideLen);
	if (MultiByteToWideChar(CP_UTF8, 0, utf8Str.c_str(), -1, wideStr.data(), wideLen) == 0) {
		return "";
	}
	int mbLen = WideCharToMultiByte(CP_ACP, 0, wideStr.data(), -1, NULL, 0, NULL, NULL);
	if (mbLen == 0) return "";
	std::vector<char> mbStr(mbLen);
	if (WideCharToMultiByte(CP_ACP, 0, wideStr.data(), -1, mbStr.data(), mbLen, NULL, NULL) == 0) {
		return "";
	}
	return std::string(mbStr.data());
}
#endif
