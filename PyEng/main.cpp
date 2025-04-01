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

#include "PyEngHostImpl.h"
#include <string>
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

#if (WIN32)
#define X_EXPORT __declspec(dllexport) 
#else
#define X_EXPORT
#include <dlfcn.h>
#endif
#include <iostream>


PyEngHost* g_pPyHost = nullptr;

enum class TraceEvent
{
	Call = 0,
	Exception = 1,
	Line = 2,
	Return = 3,
	C_Call = 4,
	C_Exception = 5,
	C_Return = 6,
	OPCode = 7
};
int x_Py_tracefunc(PyObject* self,
	struct _frame* frame,
	int event, PyObject* args)
{
	auto coFileName = g_pPyHost->Get(frame, "f_code.co_filename");
	auto szFileName = g_pPyHost->to_str(coFileName);
	std::string fileName(szFileName);
	g_pPyHost->Free(szFileName);
	auto pyLineOb = g_pPyHost->Get(frame, "f_lineno");
	int line = g_pPyHost->to_int(pyLineOb);

	TraceEvent te = (TraceEvent)event;
	switch (te)
	{
	case TraceEvent::Call:
		std::cout << "---Call----" << std::endl;
		break;
	case TraceEvent::Exception:
		std::cout << "---Exception----" << std::endl;
		break;
	case TraceEvent::Line:
		std::cout << "---Line----"<< line <<"," << fileName << std::endl;
		break;
	case TraceEvent::Return:
		std::cout << "---Return----" << std::endl;
		break;
	case TraceEvent::C_Call:
		std::cout << "---C_Call----" << std::endl;
		break;
	case TraceEvent::C_Exception:
		std::cout << "---C_Exception----" << std::endl;
		break;
	case TraceEvent::C_Return:
		std::cout << "---C_Return----" << std::endl;
		break;
	case TraceEvent::OPCode:
		std::cout << "---OPCode----" << std::endl;
		break;
	default:
		std::cout << "---Other----" << std::endl;
		break;
	}
	return 0;
}
namespace X
{
	extern XHost* g_pXHost;//defined in xload.cpp which included for PyBind.cpp
}
#if !(WIN32)
static void* __py_handle__ = nullptr;
static void PreloadPythonToSolveGlobaleExportTable()
{
	Dl_info dl_info;
	if (dladdr((void*)Py_Initialize, &dl_info)) 
	{
		const char* libpython_path = dl_info.dli_fname;
		__py_handle__ = dlopen(libpython_path, RTLD_NOW | RTLD_GLOBAL);
	}
}
static void UnloadPython()
{
	if (__py_handle__)
	{
		dlclose(__py_handle__);
	}
}
#endif
extern "C"  X_EXPORT void Load(void* pXHost,void** ppHost)
{
	X::g_pXHost = (X::XHost*)pXHost;
#if !(WIN32)
	PreloadPythonToSolveGlobaleExportTable();
#endif
	Py_Initialize();
	PyEval_InitThreads();
	PyThreadState* mainThreadState = PyEval_SaveThread();
	GrusPyEngHost::I().SetPyThreadState(mainThreadState);
	g_pPyHost = &GrusPyEngHost::I();
	*ppHost = (void*)g_pPyHost;
	//todo: add lines below back for Python debug?
	//auto ts = PyThreadState_GET();
	//PyEval_SetTrace(x_Py_tracefunc, nullptr);
}

extern "C"  X_EXPORT void Unload()
{
	//PyEval_RestoreThread(GrusPyEngHost::I().GetPyThreadState());
	X::g_pXHost = nullptr;
	Py_FinalizeEx();
#if !(WIN32)
	UnloadPython();
#endif
}