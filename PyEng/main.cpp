#include "PyEngHostImpl.h"
#include "PyEngObject.h"
#if (WIN32)
#define X_EXPORT __declspec(dllexport) 
#else
#define X_EXPORT
#endif

PyEngHost* g_pHost = nullptr;

extern "C"  X_EXPORT void Load(void** ppHost)
{
	Py_Initialize();
	g_pHost = &GrusPyEngHost::I();
	*ppHost = (void*)g_pHost;
}

extern "C"  X_EXPORT void Unload()
{
	Py_FinalizeEx();
}