#include "PyEngHostImpl.h"
#include <string>

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

extern "C"  X_EXPORT void Load(void** ppHost)
{
	Py_Initialize();
	g_pPyHost = &GrusPyEngHost::I();
	*ppHost = (void*)g_pPyHost;
	//auto ts = PyThreadState_GET();
	//PyEval_SetTrace(x_Py_tracefunc, nullptr);
}

extern "C"  X_EXPORT void Unload()
{
	Py_FinalizeEx();
}