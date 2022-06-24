#include "pycore.h"
#include "parser.h"

namespace X {

PyHandle PyLoad(char* code, int size)
{
	Parser* p = new Parser();
	p->Init();
	p->Compile(code, size);
	return (PyHandle)p;
}

bool PyRun(PyHandle h)
{
	Parser* p = (Parser*)h;
	return true;// p->Run();
}

void PyClose(PyHandle h)
{
	Parser* p = (Parser*)h;
	delete p;
}

}