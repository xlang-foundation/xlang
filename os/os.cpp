#include "fs.h"
#include "utils.h"
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
}
extern "C"  X_EXPORT void Load(void* pHost)
{
	X::g_pXHost = (X::XHost*)pHost;
	REGISTER_PACKAGE("fs", X::FileSystem);
	REGISTER_PACKAGE("utils", X::Utils);
}
extern "C"  X_EXPORT void Unload()
{
	X::g_pXHost = nullptr;
}