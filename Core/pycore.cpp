#include "pycore.h"
#include "parser.h"

namespace XPython {

PyHandle PyLoad(char* code, int size)
{
	Parser* p = new Parser();
	p->Init();
	p->Compile(code, size);
	return (PyHandle)p;
}

bool PyRun(PyHandle h)
{
	return false;
}

void PyClose(PyHandle h)
{
	Parser* p = (Parser*)h;
	delete p;
}

}