#include "xhost_impl.h"
#include "Xlang.h"
#include "str.h"
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
}