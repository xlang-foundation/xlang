#include "builtin.h"
#include "manager.h"
#include "xpackage.h"
#include "runtime.h"
#include "json.h"
#include "Hosting.h"
#include "utility.h"
#include "event.h"
#include "port.h"
#include "PyEngObject.h"
#include "xhost.h"
#include "xhost_impl.h"
#include "action.h"
#include "AddScripts.h"
#include "xload.h"
#include "Hosting.h"
#include "EventLoopInThread.h"
#include "Proxy.h"

PyEngHost* g_pPyHost = nullptr;

namespace X
{
XLoad* g_pXload = nullptr;

void UnloadPythonEngine()
{
	typedef void (*UNLOAD)();
	if (g_pXload->GetPythonLibHandler())
	{
		UNLOAD unload = (UNLOAD)GetProc(g_pXload->GetPythonLibHandler(), "Unload");
		if (unload)
		{
			unload();
		}
		UNLOADLIB(g_pXload->GetPythonLibHandler());
		g_pXload->SetPythonLibHandler(nullptr);
	}
}
void UnloadDevopsEngine()
{
	typedef void (*UNLOAD)();
	if (g_pXload->GetDevopsLibHandler())
	{
		UNLOAD unload = (UNLOAD)GetProc(g_pXload->GetDevopsLibHandler(), "Unload");
		if (unload)
		{
			unload();
		}
		UNLOADLIB(g_pXload->GetDevopsLibHandler());
		g_pXload->SetDevopsLibHandler(nullptr);
	}
}
bool LoadDevopsEngine(int port = 3141)
{
	std::string loadDllName;
	bool bHaveDll = false;
	std::vector<std::string> candiateFiles;
	std::string engName("xlang_devsrv");
	bool bRet = file_search(g_pXload->GetConfig().appPath,
		engName + ShareLibExt, candiateFiles);
	if (bRet && candiateFiles.size() > 0)
	{
		loadDllName = candiateFiles[0];
		bHaveDll = true;
	}
	else
	{
		for (auto& pa : g_pXload->GetConfig().dllSearchPath)
		{
			bRet = file_search(pa,
				engName + ShareLibExt, candiateFiles);
			if (bRet && candiateFiles.size() > 0)
			{
				loadDllName = candiateFiles[0];
				bHaveDll = true;
				break;
			}
		}
	}
	if (!bHaveDll)
	{
		return false;
	}
	typedef void (*LOAD)(void* pHost, int port);
	void* libHandle = LOADLIB(loadDllName.c_str());
	if (libHandle)
	{
		LOAD load = (LOAD)GetProc(libHandle, "Load");
		if (load)
		{
			load((void*)g_pXHost, port);
		}
		g_pXload->SetDevopsLibHandler(libHandle);
		return true;
	}
	else
	{
		return false;
	}
}
bool LoadPythonEngine()
{
	typedef void (*LOAD)(void** ppHost);

	std::string loadDllName;
	bool bHaveDll = false;
	std::vector<std::string> candiateFiles;
	std::string engName("pyeng");
	bool bRet = file_search(g_pXload->GetConfig().appPath,
		engName + ShareLibExt, candiateFiles);
	if (bRet && candiateFiles.size() > 0)
	{
		loadDllName = candiateFiles[0];
		bHaveDll = true;
	}
	else
	{
		for (auto& pa : g_pXload->GetConfig().dllSearchPath)
		{
			bRet = file_search(pa,
				engName + ShareLibExt, candiateFiles);
			if (bRet && candiateFiles.size() > 0)
			{
				loadDllName = candiateFiles[0];
				bHaveDll = true;
				break;
			}
		}
	}
	if (!bHaveDll)
	{
		return false;
	}
	if (!bHaveDll)
	{
		return false;
	}
	void* libHandle = LOADLIB(loadDllName.c_str());
	if (libHandle)
	{
		LOAD load = (LOAD)GetProc(libHandle, "Load");
		if (load)
		{
			load((void**)&g_pPyHost);
		}
		g_pXload->SetPythonLibHandler(libHandle);
		return true;
	}
	else
	{
#if (WIN32)
		DWORD err = GetLastError();
		err = err;
#endif
		return false;
	}
}
void XLangRun()
{
	if (g_pXload->GetConfig().enablePython)
	{
		LoadPythonEngine();
	}
	Builtin::I().RegisterInternals();
	BuildOps();
	if (g_pXload->GetConfig().dbg)
	{
		LoadDevopsEngine();
	}
	ScriptsManager::I().Load();
	ScriptsManager::I().Run();
	XLangProxyManager::I().Register();

	bool HasCode = false;
	std::string code;
	std::string& fileName = g_pXload->GetConfig().fileName;
	std::string& inlineCode = g_pXload->GetConfig().inlineCode;
	if (!inlineCode.empty())
	{
		Value retVal;
		fileName = "inline_code";
		HasCode = true;
		code = g_pXload->GetConfig().inlineCode;
		ReplaceAll(code, "\\n", "\n");
		ReplaceAll(code, "\\t", "\t");
		Hosting::I().Run(fileName, inlineCode.c_str(),
			(int)inlineCode.size(), retVal);
	}
	else if (!fileName.empty())
	{
		bool bOK = LoadStringFromFile(fileName, code);
		if (bOK)
		{
			HasCode = true;
		}
		else
		{
			//todo:
		}
	}
	bool enterEventLoop = g_pXload->GetConfig().enterEventLoop;
	if (HasCode)
	{
		if (g_pXload->GetConfig().runAsBackend)
		{
			Hosting::I().RunAsBackend(fileName, code);
			std::cout << "Running in background" << std::endl;
			EventSystem::I().Loop();
			enterEventLoop = false;
		}
		else
		{
			Value retVal;
			Hosting::I().Run(fileName, code.c_str(), (int)code.size(), retVal);
			if (retVal.IsValid())
			{
				std::cout << retVal.ToString() << std::endl;
			}
		}
	}
	if (enterEventLoop)//enter event loop if no file or no code
	{
		if (g_pXload->GetConfig().runEventLoopInThread)
		{
			EventLoopThread::I().Start();
		}
		else
		{
			EventSystem::I().Loop();
		}
	}
}
void XLangUnload()
{
	Builtin::I().Cleanup();
	Manager::I().Cleanup();

	if (g_pXload->GetConfig().enablePython)
	{
		UnloadPythonEngine();
	}
	if (g_pXload->GetConfig().dbg)
	{
		UnloadDevopsEngine();
	}
	if (g_pXload->GetConfig().enterEventLoop)
	{
		if (g_pXload->GetConfig().runEventLoopInThread)
		{
			EventLoopThread::I().Stop();
		}
	}
	G::I().Check();
	DestoryXHost();
}

#if __TEST__
#include "yaml_parser.h"

void test()
{
	std::string strData;
	bool bOK = LoadStringFromFile(g_pXload->GetConfig().fileName, strData);
	X::Text::YamlParser yml;
	yml.Init();
	yml.LoadFromString((char*)strData.c_str(), (int)strData.size());
	yml.Parse();

}
#endif
}

/**********************Dll Entry************************************/
#if (WIN32)
#define X_EXPORT __declspec(dllexport) 
#else
#define X_EXPORT
#endif

extern "C"  X_EXPORT void Load(void* pXload, void** pXHostHolder)
{
	std::string strFullPath;
	std::string strFolderPath;
#if (WIN32)
	HMODULE  hModule = NULL;
	GetModuleHandleEx(
		GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS,
		(LPCTSTR)Load,
		&hModule);
	char path[MAX_PATH];
	GetModuleFileName(hModule, path, MAX_PATH);
	std::string strPath(path);
	strFullPath = strPath;
	auto pos = strPath.rfind("\\");
	if (pos != std::string::npos)
	{
		strFolderPath = strPath.substr(0, pos);
	}
#else
	Dl_info dl_info;
	dladdr((void*)PyInit_russell, &dl_info);
	std::string strPath = dl_info.dli_fname;
	strFullPath = strPath;
	auto pos = strPath.rfind("/");
	if (pos != std::string::npos)
	{
		strFolderPath = strPath.substr(0, pos);
	}
#endif
	X::g_pXload = (X::XLoad*)pXload;
	X::g_pXload->GetConfig().xlangEnginePath = strFolderPath;

	auto* pXHost = X::CreatXHost();
	*pXHostHolder = pXHost;
}
extern "C"  X_EXPORT void Run()
{
	X::XLangRun();
}
extern "C"  X_EXPORT void Unload()
{
	X::XLangUnload();
}
