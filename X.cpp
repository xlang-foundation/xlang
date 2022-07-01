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



void signal_callback_handler(int signum) 
{
	X::AppEventCode code = X::Hosting::I().HandleAppEvent(signum);
	if (code == X::AppEventCode::Exit)
	{
		exit(signum);
	}
	signal(SIGINT, signal_callback_handler);
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
	X::DevOps::Debugger dbg;
	dbg.Start();

	signal(SIGINT, signal_callback_handler);
	std::string pyFileName = "C:/Dev/X/test/test2.py";
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
	dbg.Stop();
	X::Builtin::I().Cleanup();
	X::Manager::I().Cleanup();

	X::G::I().Check();
	std::cout << "End." << std::endl;
	return 0;
}
int main_b(int argc, char* argv[])
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
