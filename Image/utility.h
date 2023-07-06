#pragma once
#include <string>

std::wstring s2ws(const std::string& str);
std::string ws2s(const std::wstring& wstr);
bool GetCurLibInfo(void* EntryFuncName, std::string& strFullPath, 
	std::string& strFolderPath, std::string& strLibName);

