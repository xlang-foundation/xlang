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

// LitePy.cpp : Defines the entry point for the application.
//
#include <signal.h>
#include <vector>
#include <string>
#include <cstring>
#include "X.h"
#include "xload.h"
#include "cli.h"

#if (WIN32)
#include <Windows.h>
#include <cstdlib>
#define Path_Sep_S "\\"
#define Path_Sep '\\'
#else
#include <sys/prctl.h>
#include <string.h> //for memcpy
#define Path_Sep_S "/"
#define Path_Sep '/'
#endif


struct ParamConfig
{
	X::Config config;
	bool print_usage = false;//-help |-? |-h
	bool cli = false;
};

X::XLoad g_xLoad;
void signal_callback_handler(int signum) 
{
	X::AppEventCode code = g_xLoad.HandleAppEvent(signum);
	if (code == X::AppEventCode::Exit)
	{
#if (WIN32)
		_set_abort_behavior(0, _WRITE_ABORT_MSG); // disable error messagebox 
#else
		prctl(PR_SET_DUMPABLE, 0); // disable core dump
#endif
		abort();
	}
	signal(SIGINT, signal_callback_handler);
}


void PrintUsage()
{
	std::cout << 
"xlang [-dbg] [-enable_python|-python] \n\
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
			else if (s.find("-c ")==0)
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
				paramCfg.config.dbgPort =  std::stoi(params[i + 1]);
				i += 2;
			}
		}
		else if(!starFile)
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
				strPassInParams += "\n"+s;
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
	std::vector<std::string> params(argv, argv+argc);
	ParamConfig paramConfig;

	ParseCommandLine(params, paramConfig);
	if (paramConfig.print_usage)
	{
		PrintUsage();
		return 0;
	}
	signal(SIGINT, signal_callback_handler);
	int retCode = g_xLoad.Load(&paramConfig.config);
	if (retCode == 0)
	{
		retCode = g_xLoad.Run();
		if (paramConfig.cli)
		{
			X::CLI cli;
			cli.MainLoop();
		}
		g_xLoad.Unload();
	}
	return retCode;
}
