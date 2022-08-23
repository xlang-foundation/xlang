#include "xhost_impl.h"
#include "Xlang.h"
#include "str.h"
#include "manager.h"
#include "function.h"
#include "package.h"
#include "dict.h"
#include "bin.h"
#include "BlockStream.h"


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
	XStr* XHost_Impl::CreateStr(const char* data, int size)
	{
		Data::Str* pStrObj = data== nullptr? new Data::Str(size):new Data::Str(data, size);
		pStrObj->AddRef();
		return pStrObj;
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
	XDict* XHost_Impl::CreateDict()
	{
		auto* pDict =  new X::Data::Dict();
		pDict->AddRef();
		return pDict;
	}
	std::string XHost_Impl::StringifyString(const std::string& str)
	{
		return ::StringifyString(str);
	}
	XBin* XHost_Impl::CreateBin(char* data, size_t size)
	{
		auto* pObjBin = new Data::Binary(data, size);
		pObjBin->AddRef();
		return pObjBin;
	}
	bool XHost_Impl::ConvertToBytes(X::Value& v, X::XLStream* pStreamExternal)
	{
		X::XLangStream* pStream = nullptr;
		if (pStreamExternal == nullptr)
		{
			pStream = new X::BlockStream();
		}
		else
		{
			pStream = new XLangStream();
			pStream->SetProvider(pStreamExternal);
		}
		(*pStream) << v;
		if (pStream)
		{
			delete pStream;
		}
		return true;
	}
	bool XHost_Impl::ConvertFromBytes(X::Value& v, X::XLStream* pStreamExternal)
	{
		X::XLangStream stream;
		stream.SetProvider(pStreamExternal);
		stream >> v;
		return true;
	}
}
