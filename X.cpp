// LitePy.cpp : Defines the entry point for the application.
//
#include <signal.h>
#include <vector>
#include <string>
#include "X.h"
#include "xload.h"
#if (WIN32)
#define Path_Sep_S "\\"
#define Path_Sep '\\'
#else
#define Path_Sep_S "/"
#define Path_Sep '/'
#endif

struct ParamConfig
{
	X::Config config;
	bool print_usage = false;//-help |-? |-h
};

X::XLoad g_xLoad;
void signal_callback_handler(int signum) 
{
	X::AppEventCode code = g_xLoad.HandleAppEvent(signum);
	if (code == X::AppEventCode::Exit)
	{
		exit(signum);
	}
	signal(SIGINT, signal_callback_handler);
}


void PrintUsage()
{
	std::cout << 
"xlang [-dbg] [-enable_python|-python] \n\
      [-run_as_backend|-backend] [-event_loop]\n\
      [-c \"code,use \\n as line separator\"]\n\
      [file parameters]" << std::endl;
	std::cout << "xlang -help | -? | -h for help" << std::endl;
}
bool ParseCommandLine(std::vector<std::string>& params, ParamConfig& paramCfg)
{
	//first one is exe file name with path
	std::string& progName = params[0];
	auto pos = progName.rfind(Path_Sep);
	if (pos != progName.npos)
	{
		paramCfg.config.appPath = progName.substr(0, pos);
	}
	if (params.size() == 1)
	{
		paramCfg.print_usage = true;
		return true;
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
					paramCfg.config.inlineCode = params[i];
					i++;
				}
			}
			else if (s == "-help" || s == "-?" || s == "-h" || s == "-H")
			{
				paramCfg.print_usage = true;
				i++;
			}
			else if (s.find("-c")==0)
			{//pass code as string
				paramCfg.config.inlineCode = params[i].substr(2);
				i++;
			}
			else if (s == "-dbg")
			{
				paramCfg.config.dbg = true;
				i++;
			}
			else if (s == "-enable_python" || s == "-python")
			{
				paramCfg.config.enablePython = true;
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
		}
		else if(!starFile)
		{
			starFile = true;
			//first one is file name
			paramCfg.config.fileName = s;
			i++;
		}
		else
		{//parse passIn Params after file name
			paramCfg.config.passInParams.push_back(s);
			i++;
		}
	}
	return true;
}

int main(int argc, char* argv[])
{
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
		g_xLoad.Unload();
	}
	return retCode;
}
