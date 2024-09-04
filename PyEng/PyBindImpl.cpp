
//trick for win32 compile to avoid using pythonnn_d.lib
#ifdef _DEBUG
#undef _DEBUG
extern "C"
{
#include "Python.h"
}
#define _DEBUG
#else
extern "C"
{
#include "Python.h"
}
#endif

static PyObject*
Xlang_main(PyObject* self, PyObject* args, PyObject* kwargs)
{
	printf("inside Xlang_main\r\n");
	return PyLong_FromLong(0);
}
static PyObject*
Xlang_register(PyObject* self, PyObject* args, PyObject* kwargs)
{
	return PyLong_FromLong(0);
}

static PyObject*
Xlang_Function(PyObject* self, PyObject* args, PyObject* kwargs)
{
	return PyLong_FromLong(0);
}
static PyObject*
Xlang_Class(PyObject* self, PyObject* args, PyObject* kwargs)
{
	return PyLong_FromLong(0);
}


PyMethodDef RootMethods[] =
{
	{	"main",
		(PyCFunction)Xlang_main,
		METH_VARARGS | METH_KEYWORDS,
		"Syntax xlang.main(*args,**kwargs)"
	},
	{	"func", 
		(PyCFunction)Xlang_Function,
		METH_VARARGS|METH_KEYWORDS,
		"Syntax: wrapper = xlang.func(*args,**kwargs)" 
	},
	{	"object",
		(PyCFunction)Xlang_Class,
		METH_VARARGS | METH_KEYWORDS,
		"Syntax: wrapper = xlang.object(*args,**kwargs)"
	},
	{ NULL, NULL, 0, NULL }
};

