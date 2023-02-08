#include "xhost.h"
#include "xpackage.h"
#include "xlApp.h"
#include "androidwrapper.h"

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
	X::RegisterPackage<XWin::App>("App",&XWin::App::I());
	X::RegisterPackage<X::Android::AndroidWrapper>("App");
	XWin::App::I().SetModule(curModule);
}
extern "C"  X_EXPORT void Unload()
{
	X::g_pXHost = nullptr;
}