#include "xhost.h"
#include "xpackage.h"
#include "dbmgr.h"

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
	X::Database::Manager::I().SetModule(curModule);
	X::RegisterPackage<X::Database::Manager>("sqlite",&X::Database::Manager::I());
}
extern "C"  X_EXPORT void Unload()
{
	X::g_pXHost = nullptr;
}