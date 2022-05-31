// LitePy.cpp : Defines the entry point for the application.
//

#include "XPython.h"
#include "Core/pycore.h"

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
	std::string pyFileName = "C:/Dev/XPython/test/test1.py";
	std::ifstream pyFile(pyFileName);
	std::string code((std::istreambuf_iterator<char>(pyFile)),
		std::istreambuf_iterator<char>());

	RunCore(code);
	std::cout << "End." << std::endl;
	return 0;
}
