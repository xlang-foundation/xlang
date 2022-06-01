// LitePy.cpp : Defines the entry point for the application.
//

#include "XPython.h"
#include "Core/pycore.h"
#include "Core/builtin.h"
#include <fstream>
#include <sstream>


void RunCore(std::string& code)
{
	XPython::PyHandle h = XPython::PyLoad((char*)code.c_str(), (int)code.size());
	XPython::PyRun(h);
	XPython::PyClose(h);
}

int main(int argc, char* argv[])
{
	std::string pyFileName = "C:/Dev/XPython/test/test2.py";
	if (argc >= 2)
	{
		pyFileName = argv[1];
	}
	std::ifstream pyFile(pyFileName);
	std::string code((std::istreambuf_iterator<char>(pyFile)),
		std::istreambuf_iterator<char>());
	std::vector<std::pair<std::string, std::string>> params;
	XPython::Builtin::I().Register("print", nullptr, params);
	RunCore(code);
	std::cout << "End." << std::endl;
	return 0;
}
