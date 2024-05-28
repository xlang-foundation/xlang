#include "http.h"
#include "xhost.h"
#include "xpackage.h"
#include "cypher.h"

#if (WIN32)
#include <windows.h>
#define X_EXPORT __declspec(dllexport) 
#else
#include <dlfcn.h>
#define X_EXPORT
#endif

static bool GetCurLibInfo(void* EntryFuncName, std::string& strFullPath,
	std::string& strFolderPath, std::string& strLibName)
{
#if (WIN32)
	HMODULE  hModule = NULL;
	GetModuleHandleEx(
		GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS,
		(LPCTSTR)EntryFuncName,
		&hModule);
	char path[MAX_PATH];
	GetModuleFileName(hModule, path, MAX_PATH);
	std::string strPath(path);
	strFullPath = strPath;
	auto pos = strPath.rfind("\\");
	if (pos != std::string::npos)
	{
		strFolderPath = strPath.substr(0, pos);
		strLibName = strPath.substr(pos + 1);
	}
#else
	Dl_info dl_info;
	dladdr((void*)EntryFuncName, &dl_info);
	std::string strPath = dl_info.dli_fname;
	strFullPath = strPath;
	auto pos = strPath.rfind("/");
	if (pos != std::string::npos)
	{
		strFolderPath = strPath.substr(0, pos);
		strLibName = strPath.substr(pos + 1);
	}
#endif
	//remove ext
	pos = strLibName.rfind(".");
	if (pos != std::string::npos)
	{
		strLibName = strLibName.substr(0, pos);
	}
	return true;
}

namespace X
{
	XHost* g_pXHost = nullptr;
}
extern "C"  X_EXPORT void Load(void* pHost,X::Value curModule)
{
	std::string strFullPath;
	std::string strFolderPath;
	std::string strLibName;
	GetCurLibInfo((void*)Load, strFullPath, strFolderPath, strLibName);

	X::g_pXHost = (X::XHost*)pHost;
	X::Http::I().SetHttpModulePath(strFolderPath);
	X::Http::I().SetModule(curModule);
	X::RegisterPackage<X::Http>(strFullPath.c_str(),"http",&X::Http::I());
	X::RegisterPackage<X::Cypher>(strFullPath.c_str(), "cypher");
}
extern "C"  X_EXPORT void Unload()
{
	X::g_pXHost = nullptr;
}