
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
#include "str.h"
#include "list.h"
#include "dict.h"
#include "bin.h"
#include "metascope.h"
#include "pyproxyobject.h"
#include "moduleobject.h"
#include "BlockStream.h"
#include "future.h"
#include "tensor.h"
#include "utility.h"
#include "set.h"

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
bool LoadDevopsEngine(int port = 3142)
{
	std::string loadDllName;
	bool bHaveDll = false;
	std::vector<std::string> candiateFiles;
	std::string engName("xlang_devsrv");
	bool bRet = file_search(g_pXload->GetConfig().appPath,
		LibPrefix+engName + ShareLibExt, candiateFiles);
	if (bRet && candiateFiles.size() > 0)
	{
		loadDllName = candiateFiles[0];
		bHaveDll = true;
	}
	else if(g_pXload->GetConfig().dllSearchPath)
	{
		std::string strPaths(g_pXload->GetConfig().dllSearchPath);
		std::vector<std::string> otherSearchPaths = split(strPaths, '\n');
		for (auto& pa : otherSearchPaths)
		{
			bRet = file_search(pa,
				LibPrefix+engName + ShareLibExt, candiateFiles);
			if (bRet && candiateFiles.size() > 0)
			{
				loadDllName = candiateFiles[0];
				bHaveDll = true;
				//break;
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

PyEngObjectPtr Xlang_CallFunc_Impl(
	void* realFuncObj,
	void* pContext,
	PyEngObjectPtr args,
	PyEngObjectPtr kwargs)
{
	X::KWARGS xKwArgs;
	PyEng::Object pyArgs(args,true);
	long long cnt = pyArgs.GetCount();
	X::ARGS xArgs((int)cnt);
	for (long long i = 0; i < cnt; i++)
	{
		X::Value xVal;
		PyEng::Object pyObjArg = pyArgs[i];
		X::Data::PyProxyObject::PyObjectToValue(pyObjArg, xVal);
		xArgs.push_back(xVal);
	}
	XObj* pFuncObj = (XObj*)realFuncObj;
	auto* rt = X::g_pXHost->GetCurrentRuntime();
	X::Value retVal;
	pFuncObj->Call(rt, (XObj*)pContext, xArgs, xKwArgs, retVal);
	PyEng::Object retObj(retVal);
	return retObj;
}
bool LoadPythonEngine()
{
	typedef void (*LOAD)(void** ppHost);

	std::string loadDllName;
	bool bHaveDll = false;
	std::vector<std::string> candiateFiles;
	std::string engName("pyeng");
	bool bRet = file_search(g_pXload->GetConfig().appPath,
		LibPrefix+engName + ShareLibExt, candiateFiles);
	if (bRet && candiateFiles.size() > 0)
	{
		loadDllName = candiateFiles[0];
		bHaveDll = true;
	}
	else if (g_pXload->GetConfig().dllSearchPath)
	{
		std::string strPaths(g_pXload->GetConfig().dllSearchPath);
		std::vector<std::string> otherSearchPaths = split(strPaths, '\n');
		for (auto& pa : otherSearchPaths)
		{
			bRet = file_search(pa,
				LibPrefix+engName + ShareLibExt, candiateFiles);
			if (bRet && candiateFiles.size() > 0)
			{
				loadDllName = candiateFiles[0];
				bHaveDll = true;
				//break;
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
		if (g_pPyHost)
		{
			g_pPyHost->SetXlangCallFunc(Xlang_CallFunc_Impl);
		}
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

void XLangStaticLoad()
{
	X::CreatXHost();
	Builtin::I().RegisterInternals();
	BuildOps();
	ScriptsManager::I().Load();
	ScriptsManager::I().Run();
	XLangProxyManager::I().Register();

}
void XLangStaticRun(std::string code)
{
	X::Value retVal;
	std::vector<std::string> passInParams;
	std::string moduleName("default");
	Hosting::I().Run(moduleName, code.c_str(),
		(int)code.size(),
		passInParams,
		retVal);
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

	std::vector<std::string> passInParams;
	if (g_pXload->GetConfig().passInParams)
	{
		std::string strPassInParams(g_pXload->GetConfig().passInParams);
		passInParams = split(strPassInParams, '\n');
	}
	bool HasCode = false;
	std::string code;
	const char* fileName = g_pXload->GetConfig().fileName;
	const char* inlineCode = g_pXload->GetConfig().inlineCode;
	if (inlineCode)
	{
		Value retVal;
		std::string strFileName = "inline_code";
		HasCode = true;
		code = inlineCode;
		ReplaceAll(code, "\\n", "\n");
		ReplaceAll(code, "\\t", "\t");
		std::vector<std::string> passInParams;
		if (g_pXload->GetConfig().passInParams)
		{
			std::string strPassInParams(g_pXload->GetConfig().passInParams);
			passInParams = split(strPassInParams, '\n');
		}
		Hosting::I().Run(strFileName, inlineCode,
			(int)strlen(inlineCode),
			passInParams,
			retVal);
	}
	else if (fileName)
	{
		std::string strFileName = fileName;
		bool bOK = LoadStringFromFile(strFileName, code);
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
			std::string strFileName;
			if (fileName)
			{
				strFileName = fileName;
			}
			Hosting::I().RunAsBackend(strFileName, code,passInParams);
			std::cout << "Running in background" << std::endl;
			EventSystem::I().Loop();
			enterEventLoop = false;
		}
		else
		{
			Value retVal;
			std::string strFileName;
			if (fileName)
			{
				strFileName = fileName;
			}
			Hosting::I().Run(strFileName, code.c_str(), (int)code.size(),
				passInParams,
				retVal);
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
void XLangStaticUnload()
{
	Builtin::I().Cleanup();
	Manager::I().Cleanup();
	X::AST::ModuleObject::cleanup();
	X::Data::Str::cleanup();
	X::Data::List::cleanup();
	X::Data::Dict::cleanup();
	X::Data::Tensor::cleanup();
	X::Data::Future::cleanup();
	X::Data::Function::cleanup();
	X::AST::MetaScope().I().Cleanup();
	Hosting::I().Cleanup();
	G::I().Check();
	DestoryXHost();
}
void XLangUnload()
{
	Builtin::I().Cleanup();
	Manager::I().Cleanup();
	X::AST::ModuleObject::cleanup();
	X::Data::Str::cleanup();
	X::Data::List::cleanup();
	X::Data::Dict::cleanup();
	X::Data::mSet::cleanup();
	X::AST::MetaScope().I().Cleanup();

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
			EventSystem::I().Shutdown();
			EventLoopThread::I().Stop();
		}
	}
	Hosting::I().Cleanup();
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
	dladdr((void*)Load, &dl_info);
	std::string strPath = dl_info.dli_fname;
	strFullPath = strPath;
	auto pos = strPath.rfind("/");
	if (pos != std::string::npos)
	{
		strFolderPath = strPath.substr(0, pos);
	}
#endif
	X::g_pXload = (X::XLoad*)pXload;
	const char* engPath = new char[strFolderPath.length() + 1];
	memcpy((char*)engPath, strFolderPath.data(), strFolderPath.length() + 1);
	X::g_pXload->GetConfig().xlangEnginePath = engPath;

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
