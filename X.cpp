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

void signal_callback_handler(int signum) 
{
	X::AppEventCode code = X::Hosting::I().HandleAppEvent(signum);
	if (code == X::AppEventCode::Exit)
	{
		exit(signum);
	}
	signal(SIGINT, signal_callback_handler);
}

#include "PyEngObject.h"
PyEngHost* g_pHost = nullptr;
void LoadTest()
{
	typedef void (*LOAD)(void** ppHost);
	typedef void (*UNLOAD)();
	void* libHandle = LOADLIB("C:/Dev/X/out/build/x64-Debug/PyEng/pyeng.dll");
	if (libHandle)
	{
		LOAD load = (LOAD)GetProc(libHandle,"Load");
		UNLOAD unload = (UNLOAD)GetProc(libHandle, "Unload");
		if (load)
		{
			load((void**) &g_pHost);
		}
#if 0
		if (unload)
		{
			unload();
		}
		UNLOADLIB(libHandle);
#endif
	}
}
int main1(int argc, char* argv[])
{
	std::string jsonFileName = "C:/Dev/X/test/test.json";
	if (argc >= 2)
	{
		jsonFileName = argv[1];
	}
	std::ifstream pyFile(jsonFileName);
	std::string code((std::istreambuf_iterator<char>(
		pyFile)),std::istreambuf_iterator<char>());
	pyFile.close();
	X::Text::Json j;
	j.Init();
	j.LoadFromString((char*)code.c_str(), (int)code.size());
	std::cout << "End." << std::endl;
	return 0;
}
int main(int argc, char* argv[])
{
	//LoadTest();
	signal(SIGINT, signal_callback_handler);
	std::string pyFileName = "/mnt/c/Dev/X/Scripts/service.x";
	//std::string pyFileName = "c:/Dev/X/Scripts/service.x";
	if (argc >= 2)
	{
		pyFileName = argv[1];
	}
	std::ifstream pyFile(pyFileName);
	std::string code((std::istreambuf_iterator<char>(
		pyFile)), std::istreambuf_iterator<char>());
	pyFile.close();
	REGISTER_PACKAGE("http", X::Http)
	REGISTER_PACKAGE("fs", X::FileSystem)
	X::Builtin::I().RegisterInternals();
	X::AST::Value retVal;
	X::Hosting::I().Run(pyFileName,code.c_str(),
		(int)code.size(),retVal);
	X::Builtin::I().Cleanup();
	X::Manager::I().Cleanup();

	X::G::I().Check();
	std::cout << "End." << std::endl;
	return 0;
}

int main_dbg(int argc, char* argv[])
{
	LoadTest();

	X::DevOps::Debugger dbg;
	dbg.Start();
	signal(SIGINT, signal_callback_handler);
	REGISTER_PACKAGE("http", X::Http)
		REGISTER_PACKAGE("fs", X::FileSystem)
		X::Builtin::I().RegisterInternals();
	X::EventSystem::I().Loop();
	dbg.Stop();
	X::Builtin::I().Cleanup();
	X::Manager::I().Cleanup();

	X::G::I().Check();
	std::cout << "End." << std::endl;
	return 0;
}
int main_backend(int argc, char* argv[])
{
	std::string pyFileName = "C:/Dev/X/Scripts/service.x";
	std::ifstream pyFile(pyFileName);
	std::string code((std::istreambuf_iterator<char>(
		pyFile)), std::istreambuf_iterator<char>());
	pyFile.close();
	REGISTER_PACKAGE("http", X::Http)
	REGISTER_PACKAGE("fs", X::FileSystem)
	X::Builtin::I().RegisterInternals();

	X::Hosting::I().RunAsBackend(pyFileName,code.c_str(), (int)code.size());

	std::cout << "Running in background" << std::endl;
	std::string yes;
	std::cin >> yes;
	std::cout << "End." << std::endl;
	return 0;
}
