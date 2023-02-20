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
extern "C"  X_EXPORT void Load(void* pHost,X::Value curModule)
{
	X::g_pXHost = (X::XHost*)pHost;
	X::FileSystem::I().SetModule(curModule);
	X::RegisterPackage<X::FileSystem>("fs", &X::FileSystem::I());
	X::RegisterPackage<X::Utils>("utils");

	X::FileSystem::I().Run();
}
extern "C"  X_EXPORT void Unload()
{
	X::g_pXHost = nullptr;
}