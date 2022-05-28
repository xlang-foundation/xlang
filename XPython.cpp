// LitePy.cpp : Defines the entry point for the application.
//

#include "XPython.h"
#include "Core/pycore.h"
#include "Tools/lex.h"

#include <fstream>
#include <sstream>


static std::vector<short> _kwTree;

void RunCore(std::string& code)
{
	XPython::PyInit(&_kwTree[0]);

	XPython::PyHandle h = XPython::PyLoad((char*)code.c_str(), (int)code.size());
	XPython::PyRun(h);
	XPython::PyClose(h);
}
void RunTools()
{
	MakeLexTree(_kwTree);
}
int main(int argc, char* argv[])
{
	RunTools();

	std::string pyFileName = "C:/Dev/XPython/test/test1.py";
	std::ifstream pyFile(pyFileName);
	std::string code((std::istreambuf_iterator<char>(pyFile)),
		std::istreambuf_iterator<char>());

	RunCore(code);
	std::cout << "End." << std::endl;
	return 0;
}
