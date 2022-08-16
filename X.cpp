// LitePy.cpp : Defines the entry point for the application.
//

#include "X.h"
#include "Core/builtin.h"
#include <fstream>
#include <sstream>
#include "manager.h"
#include "http.h"
#include "xlang.h"
#include "runtime.h"
#include "json.h"
#include "Hosting.h"
#include "fs.h"
#include <signal.h>
#include "utility.h"
#include "devops.h"
#include "event.h"
#include "port.h"
#include "PyEngObject.h"

struct ParamConfig
{
	bool dbg = false;//-dbg
	bool enablePython = false;//-enable_python
	bool runAsBackend = false;//-run_as_backend
	std::string inlineCode;//-c "code"
	std::string fileName;
	std::vector<std::string> passInParams;


	//context
	void* pythonLibHandle = nullptr;
};

PyEngHost* g_pHost = nullptr;
std::string g_ExePath;
ParamConfig g_ParamConfig;



void signal_callback_handler(int signum) 
{
	X::AppEventCode code = X::Hosting::I().HandleAppEvent(signum);
	if (code == X::AppEventCode::Exit)
	{
		exit(signum);
	}
	signal(SIGINT, signal_callback_handler);
}

void UnloadPythonEngine()
{
	typedef void (*UNLOAD)();
	if (g_ParamConfig.pythonLibHandle)
	{
		UNLOAD unload = (UNLOAD)GetProc(g_ParamConfig.pythonLibHandle, "Unload");
		if (unload)
		{
			unload();
		}
		UNLOADLIB(g_ParamConfig.pythonLibHandle);
		g_ParamConfig.pythonLibHandle = nullptr;
	}
}
bool LoadPythonEngine()
{
	typedef void (*LOAD)(void** ppHost);

	std::string engFilePath = g_ExePath+Path_Sep_S+"pyeng.dll";
	if (!exists(engFilePath))
	{
		engFilePath = g_ExePath + Path_Sep_S +"PyEng"+ Path_Sep_S+"pyeng.dll";
		if (!exists(engFilePath))
		{
			return false;
		}
	}
	void* libHandle = LOADLIB(engFilePath.c_str());
	if (libHandle)
	{
		LOAD load = (LOAD)GetProc(libHandle,"Load");
		if (load)
		{
			load((void**) &g_pHost);
		}
		g_ParamConfig.pythonLibHandle = libHandle;
		return true;
	}
	else
	{
		return false;
	}
}

bool ParseCommandLine(std::vector<std::string>& params, ParamConfig& paramCfg)
{
	//first one is exe file name with path
	std::string& progName = params[0];
	auto pos = progName.rfind(Path_Sep);
	if (pos != progName.npos)
	{
		g_ExePath = progName.substr(0, pos);
	}
	int i = 1;
	bool starFile = false;
	while(i< (int)params.size())
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
					paramCfg.inlineCode = params[i];
					i++;
				}
			}
			else if (s.starts_with("-c"))
			{//pass code as string
				paramCfg.inlineCode = params[i].substr(2);
				i++;
			}
			else if (s == "-dbg")
			{
				paramCfg.dbg = true;
				i++;
			}
			else if (s == "-enable_python")
			{
				paramCfg.enablePython = true;
				i++;
			}
			else if (s == "-run_as_backend")
			{
				paramCfg.runAsBackend = true;
				i++;
			}
		}
		else if(!starFile)
		{
			starFile = true;
			//first one is file name
			paramCfg.fileName = s;
			i++;
		}
		else
		{//parse passIn Params after file name
			paramCfg.passInParams.push_back(s);
			i++;
		}
	}
	return true;
}

bool LoadCodeFromFile(std::string& fileName, std::string& codeInFile)
{
	std::ifstream pyFile(fileName);
	std::string code((std::istreambuf_iterator<char>(
		pyFile)), std::istreambuf_iterator<char>());
	pyFile.close();
	codeInFile = code;
	return true;
}

int main(int argc, char* argv[])
{
	signal(SIGINT, signal_callback_handler);

	std::vector<std::string> params(argv, argv+argc);
	ParseCommandLine(params, g_ParamConfig);
	if (g_ParamConfig.enablePython)
	{
		LoadPythonEngine();
	}
	X::DevOps::Debugger* dbg=nullptr;
	if (g_ParamConfig.dbg)
	{
		dbg = new X::DevOps::Debugger();
		dbg->Init();
	}
	REGISTER_PACKAGE("http", X::Http)
	REGISTER_PACKAGE("fs", X::FileSystem)
	X::Builtin::I().RegisterInternals();

	bool HasCode = false;
	std::string code;
	std::string fileName;
	if (!g_ParamConfig.inlineCode.empty())
	{
		X::AST::Value retVal;
		fileName = "inline_code";
		HasCode = true;
		code = g_ParamConfig.inlineCode;
		X::Hosting::I().Run(fileName, g_ParamConfig.inlineCode.c_str(),
			(int)g_ParamConfig.inlineCode.size(), retVal);
	}
	else if (!g_ParamConfig.fileName.empty())
	{
		bool bOK = LoadCodeFromFile(g_ParamConfig.fileName, code);
		if (bOK)
		{
			HasCode = true;
			fileName = g_ParamConfig.fileName;
		}
		else
		{
			//todo:
		}
	}
	if (HasCode)
	{
		if (g_ParamConfig.runAsBackend)
		{
			X::Hosting::I().RunAsBackend(fileName, code.c_str(), (int)code.size());
			std::cout << "Running in background" << std::endl;
			X::EventSystem::I().Loop();
		}
		else
		{
			X::AST::Value retVal;
			X::Hosting::I().Run(fileName, code.c_str(), (int)code.size(), retVal);
			std::cout << "RetVal:" << retVal.ToString() << std::endl;
		}
	}
	else//enter event loop if no file or no code
	{
		X::EventSystem::I().Loop();
	}
	if (g_ParamConfig.dbg)
	{
		dbg->Uninit();
	}
	X::Builtin::I().Cleanup();
	X::Manager::I().Cleanup();

	if (g_ParamConfig.enablePython)
	{
		UnloadPythonEngine();
	}
	X::G::I().Check();
	return 0;
}
