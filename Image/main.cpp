#include "xhost.h"
#include "xpackage.h"
#include "xlFactory.h"
#include "utility.h"

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
	std::string strFullPath;
	std::string strFolderPath;
	std::string strLibName;
	GetCurLibInfo(Load, strFullPath, strFolderPath, strLibName);

	X::g_pXHost = (X::XHost*)pHost;
	X::Images::Factory::I().SetModule(curModule);
	X::RegisterPackage<X::Images::Factory>(strFullPath.c_str(),"Factory");
}
extern "C"  X_EXPORT void Unload()
{
	X::g_pXHost = nullptr;
}