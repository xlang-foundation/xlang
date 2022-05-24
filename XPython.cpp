// LitePy.cpp : Defines the entry point for the application.
//

#include "XPython.h"
#include "Core/pycore.h"
#include "Tools/lex.h"


static std::vector<short> _kwTree;

void RunCore()
{
	std::string code =
		"st='this is \"a\" string'\
		s0 =\"another strng\"\
		x=1\
		def func1(ddd:str,100:int):\
			y=2\
			z=x+y";
	PyInit(&_kwTree[0]);

	PyHandle h = PyLoad((char*)code.c_str(), (int)code.size());
	PyRun(h);
	PyClose(h);
}
void RunTools()
{
	MakeLexTree(_kwTree);
}
int main()
{
	RunTools();

	RunCore();
	std::cout << "Hello CMake." << std::endl;
	return 0;
}
