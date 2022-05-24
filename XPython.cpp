// LitePy.cpp : Defines the entry point for the application.
//

#include "XPython.h"
#include "Core/pycore.h"
#include "Tools/lex.h"


static std::vector<short> _kwTree;

void RunCore()
{
	std::string code =
		"k=1.2345\
		st='this is \"a\" string'\
		s0 =\"another strng\"\
		x=1\
		def func1(ddd:str,i1:int):\
			y=2\
			z=x+y\
		var1 = func1(st,100)";
	std::string code0 = "var1 = func1(st,100)";

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
	std::cout << "End." << std::endl;
	return 0;
}
