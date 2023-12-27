#include "xload.h"
#include "xpackage.h"
#include "xlang.h"
#include "apibridge.h"

#if (WIN32)
#include <windows.h>
#define X_EXPORT __declspec(dllexport) 
#else
#include <dlfcn.h>
#define X_EXPORT
#endif

static bool GetCurLibInfo(void* EntryFuncName, std::string& strFullPath,
	std::string& strFolderPath, std::string& strLibName)
{
#if (WIN32)
	HMODULE  hModule = NULL;
	GetModuleHandleEx(
		GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS,
		(LPCTSTR)EntryFuncName,
		&hModule);
	char path[MAX_PATH];
	GetModuleFileName(hModule, path, MAX_PATH);
	std::string strPath(path);
	strFullPath = strPath;
	auto pos = strPath.rfind("\\");
	if (pos != std::string::npos)
	{
		strFolderPath = strPath.substr(0, pos);
		strLibName = strPath.substr(pos + 1);
	}
#else
	Dl_info dl_info;
	dladdr((void*)EntryFuncName, &dl_info);
	std::string strPath = dl_info.dli_fname;
	strFullPath = strPath;
	auto pos = strPath.rfind("/");
	if (pos != std::string::npos)
	{
		strFolderPath = strPath.substr(0, pos);
		strLibName = strPath.substr(pos + 1);
	}
#endif
	//remove ext
	pos = strLibName.rfind(".");
	if (pos != std::string::npos)
	{
		strLibName = strLibName.substr(0, pos);
	}
	return true;
}

std::string g_strModuleFullPath;

struct XLanEngineContext
{
	X::XLoad xLoad;
	X::Config config;
};


namespace X
{ 
	namespace Interop
	{
		CreateOrGetClassInstanceDelegate g_createOrGetClassInstanceDelegate = nullptr;
		InvokeMethodDelegate g_invokeMethodDelegate = nullptr;
	}
}


extern "C"  X_EXPORT bool CallObjectFunc(void* pObjPtr,const char* funcName,
	X::Value* variantArray,int arrayLength,X::Value* outReturnValue)
{
	return true;
}

//if return value is string, need to consider memory allocation and deallocation
//so need to use sepreated function to call paring with string releease function

extern "C"  X_EXPORT const char* CallObjectToString(void* pObjPtr)
{
	X::XObj* pXObj = (X::XObj*)pObjPtr;
	const char* str = pXObj->ToString();
	return str;
}

extern "C"  X_EXPORT void ReleaseString(const char* str)
{
	delete str;
}

extern "C"  X_EXPORT bool CreateAPISet(void** ppApiBridge)
{
	auto* pApiBridge = new X::Interop::ApiBridge();
	*ppApiBridge = (void*)pApiBridge;
	return true;
}

extern "C"  X_EXPORT bool AddApi(void* pVoidApiBridge,X::PackageMemberType type,const char* name)
{
	auto* pApiBridge = (X::Interop::ApiBridge*)pVoidApiBridge;
	return pApiBridge->RegisterApi(type,name);
}

extern "C"  X_EXPORT void DeleteAPISet(void* pVoidApiBridge)
{
	auto* pApiBridge = (X::Interop::ApiBridge*)pVoidApiBridge;
	delete pApiBridge;
}
extern "C"  X_EXPORT bool RegisterPackage(const char* className,
	void* pVoidApiBridge,bool singleInstance,void* createFuncOrInstance)
{
	auto* pApiBridge = (X::Interop::ApiBridge*)pVoidApiBridge;
	pApiBridge->SetCreateFuncOrInstance(singleInstance, createFuncOrInstance);
	pApiBridge->CreatePackage();
	if (singleInstance)
	{
		pApiBridge->AddMap(createFuncOrInstance);
		auto* pXPack = pApiBridge->APISET().GetPack();
		//when call CreatePackage, we passed the pApiBridge to the package as the embed object
		//but we need to keep same logic as passing creation function
		//so change to set embed object here
		pXPack->SetEmbedObj(createFuncOrInstance);
		X::Value v0(dynamic_cast<X::XObj*>(pXPack));
		X::g_pXHost->RegisterPackage(className, v0);
	}
	else
	{
		X::g_pXHost->RegisterPackage(className, [](X::Value context)
			{
				auto* pApiBridge = (X::Interop::ApiBridge*)(void*)context;
				return pApiBridge->CreatePackProxy();
			}, (void*)pApiBridge);
	}
	//return apiset.GetPack();
	return true;
}
extern "C"  X_EXPORT bool Load(void* createCallback,void* invokeCallback,void** ppContext)
{
	X::Interop::g_createOrGetClassInstanceDelegate = (X::Interop::CreateOrGetClassInstanceDelegate)createCallback;
	X::Interop::g_invokeMethodDelegate = (X::Interop::InvokeMethodDelegate)invokeCallback;

	std::string strFullPath;
	std::string strLibName;
	std::string strFolderPath;
	GetCurLibInfo((void*)Load, strFullPath, strFolderPath, strLibName);
	g_strModuleFullPath = strFullPath;
	XLanEngineContext* pContext = new XLanEngineContext();
	X::XLoad& xLoad = pContext->xLoad;
	X::Config& config = pContext->config;
	pContext->config.appPath = strFolderPath.c_str();
	pContext->config.appFullName = strFullPath.c_str();

	config.dbg = true;
	config.runEventLoopInThread = true;
	config.enterEventLoop = true;
	int retCode = xLoad.Load(&config);
	if (retCode == 0) 
	{
		xLoad.Run();
		*ppContext = pContext;
		return true;
	}
	else
	{
		delete pContext;
		*ppContext = nullptr;
		return false;
	}
}
extern "C"  X_EXPORT void Unload(void* pContext)
{
	if (pContext == nullptr)
	{
		return;
	}
	auto* pXLanEngineContext = (XLanEngineContext*)pContext;
	X::XLoad& xLoad = pXLanEngineContext->xLoad;
	xLoad.Unload();
	delete pXLanEngineContext;
}

extern "C" X_EXPORT bool FireObjectEvent(void* objPtr, int evtId, 
	X::Value * variantArray, int arrayLength)
{
	auto* pBridge = X::Interop::ApiBridge::GetApiBridge(objPtr);
	if (pBridge == nullptr)
	{
		return false;
	}
	return pBridge->FireObjectEvent(evtId, variantArray, arrayLength);
}
