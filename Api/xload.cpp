#include "xload.h"
#include "xhost.h"

#if (WIN32)
#include <Windows.h>
#define Path_Sep_S "\\"
#define Path_Sep '\\'
#define ShareLibExt ".dll"
#define LOADLIB(path) LoadLibraryEx(path,NULL,LOAD_WITH_ALTERED_SEARCH_PATH)
#define GetProc(handle,funcName) GetProcAddress((HMODULE)handle, funcName)
#define UNLOADLIB(h) FreeLibrary((HMODULE)h)
#else
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/wait.h>
#include <dlfcn.h>
#define Path_Sep_S "/"
#define Path_Sep '/'
#define ShareLibExt ".so"
#define LOADLIB(path) dlopen(path, RTLD_LAZY)
#define GetProc(handle,funcName) dlsym(handle, funcName)
#define UNLOADLIB(handle) dlclose(handle)
#endif


namespace X
{
	static bool dir(std::string search_pat,
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
	static bool file_search(std::string folder,
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
	XHost* g_pXHost = nullptr;
	XLoad::XLoad()
	{

	}
	AppEventCode XLoad::HandleAppEvent(int signum)
	{
		return g_pXHost?g_pXHost->HandleAppEvent(signum): AppEventCode::Error;
	}
	static bool SearchDll(std::string& dllMainName, std::string& searchPath,
		std::string& findFileName)
	{
		bool bHaveDll = false;
		std::vector<std::string> candiateFiles;
		bool bRet = file_search(searchPath,
			dllMainName + ShareLibExt, candiateFiles,false);
		if (bRet && candiateFiles.size() > 0)
		{
			findFileName = candiateFiles[0];
			bHaveDll = true;
		}
		return bHaveDll;
	}
	void XLoad::Unload()
	{
		if (xlangLibHandler)
		{
			typedef void (*Unload)();
			Unload unload = (Unload)GetProc(xlangLibHandler, "Unload");
			if (unload)
			{
				unload();
			}
			UNLOADLIB(xlangLibHandler);
			xlangLibHandler = nullptr;
		}
	}
	int XLoad::Load(Config* pCfg)
	{
		m_pConfig = pCfg;
		std::string engName("xlang_eng");
		std::string loadDllName;
		bool bFound = SearchDll(engName, m_pConfig->appPath, loadDllName);
		if (!bFound)
		{
			for (auto pa : m_pConfig->dllSearchPath)
			{
				bFound = SearchDll(engName,pa, loadDllName);
				if (bFound)
				{
					break;
				}
			}
		}
		if (!bFound)
		{
			return -1;
		}
		typedef void (*LOAD)(void* pXload,void** pXHost);
		void* libHandle = LOADLIB(loadDllName.c_str());
		if (libHandle)
		{
			LOAD load = (LOAD)GetProc(libHandle, "Load");
			if (load)
			{
				load((void*)this,(void**)&g_pXHost);
			}
			SetXLangLibHandler(libHandle);
			return 0;
		}
		return -1;
	}
	int XLoad::Run()
	{
		if (xlangLibHandler == nullptr)
		{
			return -1;
		}
		typedef void (*XRun)();
		XRun run = (XRun)GetProc(xlangLibHandler, "Run");
		if (run)
		{
			run();
			return 0;
		}
		else
		{
			return -1;
		}
	}
}