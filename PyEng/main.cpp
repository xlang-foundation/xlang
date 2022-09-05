#include "PyEngHostImpl.h"
#include "PyEngObject.h"
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
	PyEng::Object objFrame(frame,true);
	auto fileName = (std::string)objFrame["f_code.co_filename"];
	int line = (int)objFrame["f_lineno"];
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