#include "pycore.h"
#include "parser.h"

namespace XPython {

static short* _kwTree = nil;
void PyInit(short* kwTree)
{
	_kwTree = kwTree;
}
PyHandle PyLoad(char* code, int size)
{
	Parser* p = new Parser();
	p->Init(_kwTree);
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