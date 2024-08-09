#include <signal.h>
#include "PyEngHost.h"
#include "xlang.h"
#include "xload.h"
#include "utility.h"
#include "port.h"
#include <vector>
#include "PyEngObject.h"
#include "pyproxyobject.h"
#include "cli.h"
#include "Hosting.h"
#include "action.h"
#include "xhost_impl.h"
#include "builtin.h"
#include "AddScripts.h"
#include "Proxy.h"
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
#include "deferred_object.h"
#include "typeobject.h"
#include "tensor.h"
#include "tensor_graph.h"
#include "manager.h"
#include "struct.h"

//for OS Module
#include "os/fs.h"
#include "os/utils.h"
#include "os/process.h"


#if (WIN32)
#include <Windows.h>
#define Path_Sep_S "\\"
#define Path_Sep '\\'
#else
#include <string.h> //for memcpy
#define Path_Sep_S "/"
#define Path_Sep '/'
#endif


PyEngHost* g_pPyHost = nullptr;

namespace X
{
	XLoad* g_pXload = nullptr;

	PyEngObjectPtr Xlang_CallFunc_Impl(
		void* realFuncObj,
		void* pContext,
		PyEngObjectPtr args,
		PyEngObjectPtr kwargs)
	{
		X::KWARGS xKwArgs;
		PyEng::Object pyArgs(args, true);
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
			LibPrefix + engName + ShareLibExt, candiateFiles);
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
					LibPrefix + engName + ShareLibExt, candiateFiles);
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
		X::Builtin::I().RegisterInternals();
		X::BuildOps();
		X::ScriptsManager::I().Load();
		X::ScriptsManager::I().Run();
		X::XLangProxyManager::I().Register();

	}
	static void XLangInternalInit()
	{
		X::Data::Str::Init();
		X::AST::ModuleObject::Init();
		X::Data::List::Init();
		X::Data::Binary::Init();
		X::Data::Dict::Init();
		X::Data::mSet::Init();
		X::Data::Tensor::Init();
		X::Data::TensorGraph::Init();
		X::AST::MetaScope().I().Init();
		X::Data::DeferredObject::Init();
		X::Data::TypeObject::Init();
		X::Data::XlangStruct::Init();
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
			LibPrefix + engName + ShareLibExt, candiateFiles);
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
					LibPrefix + engName + ShareLibExt, candiateFiles);
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

	void XLangRun(X::Config& config)
	{
		XLangInternalInit();
		if (config.enablePython)
		{
			LoadPythonEngine();
		}
		Builtin::I().RegisterInternals();
		BuildOps();
		if (config.dbg)
		{
			LoadDevopsEngine();
		}
		ScriptsManager::I().Load();
		ScriptsManager::I().Run();
		XLangProxyManager::I().Register();

		std::vector<X::Value> passInParams;
		if (config.passInParams)
		{
			std::string strPassInParams(config.passInParams);
			auto strParams = split(strPassInParams, '\n');
			for (auto& s : strParams)
			{
				passInParams.push_back(s);
			}
		}
		bool HasCode = false;
		std::string code;
		const char* fileName = config.fileName;
		const char* inlineCode = config.inlineCode;
		if (inlineCode)
		{
			Value retVal;
			HasCode = true;
			code = inlineCode;
			ReplaceAll(code, "\\n", "\n");
			ReplaceAll(code, "\\t", "\t");
			std::vector<X::Value> passInParams;
			if (config.passInParams)
			{
				std::string strPassInParams(g_pXload->GetConfig().passInParams);
				auto strParams = split(strPassInParams, '\n');
				for (auto& s : strParams)
				{
					passInParams.push_back(s);
				}
			}
			Hosting::I().Run("inline_code", inlineCode,
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
		bool enterEventLoop = config.enterEventLoop;
		if (HasCode)
		{
			if (config.runAsBackend)
			{
				std::string strFileName;
				if (fileName)
				{
					strFileName = fileName;
				}
				int pSize = (int)passInParams.size();
				std::vector<X::Value> params;
				for (int i = 0; i < pSize; i++)
				{
					params.push_back(passInParams[i]);
				}
				Hosting::I().RunAsBackend(strFileName, code, params);
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
				Hosting::I().Run(strFileName.c_str(), code.c_str(), (int)code.size(),
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
			if (config.runEventLoopInThread)
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
		X::Data::Binary::cleanup();
		X::Data::Dict::cleanup();
		X::Data::Tensor::cleanup();
		X::Data::TensorGraph::cleanup();
		X::Data::Future::cleanup();
		X::Data::Function::cleanup();
		X::Data::DeferredObject::cleanup();
		X::Data::TypeObject::cleanup();
		X::AST::MetaScope().I().Cleanup();
		X::Data::XlangStruct::cleanup();
		Hosting::I().Cleanup();
		G::I().Check();
		DestoryXHost();
	}

	//For OS Module
	void RegOSModule(std::string& path)
	{
		X::FileSystem::I().SetModulePath(path);
		X::RegisterPackage<X::FileSystem>("xone", "fs", &X::FileSystem::I());
		X::RegisterPackage<X::Utils>("xone", "utils");
		X::RegisterPackage<X::OSService>("xone", "os", &X::OSService::I());

		X::FileSystem::I().Run();
	}

} //namespace X



struct ParamConfig
{
	X::Config config;
	bool print_usage = false;//-help |-? |-h
	bool cli = false;
};


void signal_callback_handler(int signum)
{
	//X::AppEventCode code = g_xLoad.HandleAppEvent(signum);
	//if (code == X::AppEventCode::Exit)
	//{
	//	exit(signum);
	//}
	//signal(SIGINT, signal_callback_handler);
}


void PrintUsage()
{
	std::cout <<
		"xone [-dbg] [-enable_python|-python] \n\
      [-run_as_backend|-backend] [-event_loop]\n\
      [-c \"code,use \\n as line separator\"]\n\
      [-cli]\n\
      [file parameters]" << std::endl;
	std::cout << "xlang -help | -? | -h for help" << std::endl;
}
bool ParseCommandLine(std::vector<std::string>& params, ParamConfig& paramCfg)
{
	//first one is exe file name with path
	std::string progName = params[0];
	std::string strAppPath;
	auto pos = progName.rfind(Path_Sep);
	if (pos != progName.npos)
	{
		strAppPath = progName.substr(0, pos);
	}
#if (WIN32)
	else
	{
		char buffer[MAX_PATH];
		DWORD length = GetModuleFileName(nullptr, buffer, MAX_PATH);
		progName = buffer;
		auto pos = progName.rfind(Path_Sep);
		if (pos != progName.npos)
		{
			strAppPath = progName.substr(0, pos);
		}
		else
		{
			strAppPath = "";
		}
	}
#endif
	paramCfg.config.appPath = new char[strAppPath.length() + 1];
	memcpy((char*)paramCfg.config.appPath, strAppPath.data(), strAppPath.length() + 1);
	paramCfg.config.appFullName = new char[progName.length() + 1];
	memcpy((char*)paramCfg.config.appFullName, progName.data(), progName.length() + 1);



	if (params.size() == 1)
	{
		paramCfg.print_usage = true;
		return true;
	}
	std::string strPassInParams;
	int i = 1;
	bool starFile = false;
	while (i < (int)params.size())
	{
		std::string& s = params[i];
		if (s.size() == 0)
		{
			continue;
		}
		if (!starFile && s[0] == '-')
		{
			if (s == "-c")
			{//pass code as string
				i++;
				if (i < (int)params.size())
				{
					auto& s_i = params[i];
					paramCfg.config.inlineCode = new char[s_i.length() + 1];
					memcpy((char*)paramCfg.config.inlineCode, s_i.data(), s_i.length() + 1);
					i++;
				}
			}
			else if (s == "-help" || s == "-?" || s == "-h" || s == "-H")
			{
				paramCfg.print_usage = true;
				i++;
			}
			else if (s.find("-c ") == 0)
			{//pass code as string
				auto s_i = params[i].substr(2);
				paramCfg.config.inlineCode = new char[s_i.length() + 1];
				memcpy((char*)paramCfg.config.inlineCode, s_i.data(), s_i.length() + 1);
				i++;
			}
			else if (s == "-dbg")
			{
				paramCfg.config.dbg = true;
				i++;
			}
			else if (s == "-cli")
			{
				paramCfg.cli = true;
				i++;
			}
			else if (s == "-enable_python" || s == "-python")
			{
				paramCfg.config.enablePython = true;
				i++;
			}
			else if (s == "-enable_python_debug" || s == "-python_debug")
			{
				paramCfg.config.enablePythonDebug = true;
				i++;
			}
			else if (s == "-run_as_backend" || s == "-backend")
			{
				paramCfg.config.runAsBackend = true;
				i++;
			}
			else if (s == "-event_loop")
			{
				paramCfg.config.enterEventLoop = true;
				i++;
			}
			else if (s == "-port")
			{
				paramCfg.config.dbgPort = std::stoi(params[i + 1]);
				i += 2;
			}
		}
		else if (!starFile)
		{
			starFile = true;
			//first one is file name
			paramCfg.config.fileName = new char[s.length() + 1];
			memcpy((char*)paramCfg.config.fileName, s.data(), s.length() + 1);
			i++;
		}
		else
		{//parse passIn Params after file name
			if (strPassInParams.empty())
			{
				strPassInParams = s;
			}
			else
			{
				strPassInParams += "\n" + s;
			}
			i++;
		}
	}
	if (!strPassInParams.empty())
	{
		paramCfg.config.passInParams = new char[strPassInParams.length() + 1];
		memcpy((char*)paramCfg.config.passInParams, strPassInParams.data(), strPassInParams.length() + 1);
	}
	return true;
}

#include <thread>
void func()
{
}
void Workaround_WSLThread_Problem()
{
	static std::vector<std::thread> threads_;
	threads_.emplace_back(func);
}


int main(int argc, char* argv[])
{
	//::MessageBox(NULL, "In Dbg", "XLang", MB_OK);
	//Workaround_WSLThread_Problem();
	std::vector<std::string> params(argv, argv + argc);
	ParamConfig paramConfig;

	ParseCommandLine(params, paramConfig);
	if (paramConfig.print_usage)
	{
		PrintUsage();
		return 0;
	}
	std::string appPath(paramConfig.config.appPath);

	signal(SIGINT, signal_callback_handler);
	X::XLangStaticLoad();
	X::RegOSModule(appPath);
	X::XLangRun(paramConfig.config);
	if (paramConfig.cli)
	{
		X::CLI cli;
		cli.MainLoop();
	}
	X::XLangStaticUnload();
	return 0;
}
