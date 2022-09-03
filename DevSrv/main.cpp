#include "devsrv.h"
#include "xhost.h"
#include "xpackage.h"

#if (WIN32)
#define X_EXPORT __declspec(dllexport) 
#else
#define X_EXPORT
#endif

namespace X
{
	XHost* g_pXHost = nullptr;
	X::DevServer* g_pDevOps = nullptr;
}
extern "C"  X_EXPORT void Load(void* pHost,int port)
{
	X::g_pXHost = (X::XHost*)pHost;
	X::g_pDevOps = new X::DevServer(port);
	X::g_pDevOps->Start();
}
extern "C"  X_EXPORT void Unload()
{
	if (X::g_pDevOps)
	{
		X::g_pDevOps->Stop();
		delete X::g_pDevOps;
	}
	X::g_pXHost = nullptr;
}