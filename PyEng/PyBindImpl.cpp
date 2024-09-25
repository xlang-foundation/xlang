/*
Copyright (C) 2024 The XLang Foundation
Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

#include "xlang.h"
#include "xhost.h"

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


extern PyObject* CreateXlangObjectWrapper(X::Value& realObj);

static PyObject*
Xlang_main(PyObject* self, PyObject* args, PyObject* kwargs)
{
	printf("inside Xlang_main\r\n");
	return PyLong_FromLong(0);
}
static PyObject*
Xlang_import(PyObject* self, PyObject* args, PyObject* kwargs)
{
	const char* moduleName = nullptr;
	const char* from = nullptr;
	const char* thru = nullptr;
	static char* kwlist[] = { "moduleName", "from", "thru", nullptr };

	if (!PyArg_ParseTupleAndKeywords(args, kwargs, "s|ss", kwlist, &moduleName, &from, &thru))
	{
		return nullptr;
	}

	auto* rt = X::g_pXHost->GetCurrentRuntime();
	X::Value obj;
	bool bOK = false;
	Py_BEGIN_ALLOW_THREADS
		bOK = X::g_pXHost->Import(rt, moduleName, from, thru, obj);
	Py_END_ALLOW_THREADS
	if (!bOK)
	{
		PyErr_SetString(PyExc_RuntimeError, "Import failed");
		return nullptr;
	}
	return CreateXlangObjectWrapper(obj);
}


static PyObject*
Xlang_Function(PyObject* self, PyObject* args, PyObject* kwargs)
{
	Py_ssize_t size = PyTuple_Size(args);
	for (Py_ssize_t i = 0; i < size; ++i) {
		PyObject* item = PyTuple_GetItem(args, i);
		item = item;
	}
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
	{	"importModule",
		(PyCFunction)Xlang_import,
		METH_VARARGS | METH_KEYWORDS,
		"Syntax: wrapper = xlang.importModule(*args,**kwargs)"
	},
	{	"object",
		(PyCFunction)Xlang_Class,
		METH_VARARGS | METH_KEYWORDS,
		"Syntax: wrapper = xlang.object(*args,**kwargs)"
	},
	{ NULL, NULL, 0, NULL }
};

