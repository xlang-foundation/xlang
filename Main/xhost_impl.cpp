#include "xhost_impl.h"
#include "Xlang.h"
#include "str.h"
#include "manager.h"
#include "function.h"
#include "package.h"

namespace X 
{
	X::XHost* g_pXHost = nullptr;
	void CreatXHost()
	{
		g_pXHost = new XHost_Impl();
	}
	void DestoryXHost()
	{
		if (g_pXHost)
		{
			delete g_pXHost;
		}
	}
	XObj* XHost_Impl::CreateStrObj(const char* data, int size)
	{
		Data::Str* pStrObj = new Data::Str(data, size);
		pStrObj->AddRef();
		return dynamic_cast<XObj*>(pStrObj);
	}
	bool XHost_Impl::RegisterPackage(const char* name, PackageCreator creator)
	{
		return X::Manager::I().Register(name, creator);
	}
	XPackage* XHost_Impl::CreatePackage(void* pRealObj)
	{
		auto* pPack = new AST::Package(pRealObj);
		pPack->AddRef();
		return pPack;
	}
	XFunc* XHost_Impl::CreateFunction(const char* name,U_FUNC func)
	{
		std::string strName(name);
		AST::ExternFunc* extFunc = new AST::ExternFunc(strName, func);
		auto* pFuncObj = new X::Data::Function(extFunc);
		pFuncObj->AddRef();
		return dynamic_cast<XFunc*>(pFuncObj);
	}
	/*   TODO:
		some pointer convert to Data::Object and pass to Value
		have to use this way to get right Object

	*/
	XObj* XHost_Impl::ConvertObjFromPointer(void* pObjectPtr)
	{
		return (XObj*)(X::Data::Object*)(pObjectPtr);
	}
}
