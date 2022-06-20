// LitePy.cpp : Defines the entry point for the application.
//

#include "X.h"
#include "Core/pycore.h"
#include "Core/builtin.h"
#include <fstream>
#include <sstream>
#include "manager.h"
#include "http.h"
#include "xlang.h"
#include "runtime.h"

void RunCore(std::string& code)
{
	X::PyHandle h = X::PyLoad((char*)code.c_str(), (int)code.size());
	X::PyRun(h);
	X::PyClose(h);
}

int main(int argc, char* argv[])
{
	std::string pyFileName = "C:/Dev/X/test/test2.py";
	if (argc >= 2)
	{
		pyFileName = argv[1];
	}
	std::ifstream pyFile(pyFileName);
	std::string code((std::istreambuf_iterator<char>(
		pyFile)),std::istreambuf_iterator<char>());
	pyFile.close();
	REGISTER_PACKAGE("http", X::Http)
	X::Builtin::I().RegisterInternals();
	RunCore(code);
	std::cout << "End." << std::endl;
	return 0;
}
