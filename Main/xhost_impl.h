#pragma once
#include "xhost.h"

namespace X
{
	class XHost_Impl :
		public XHost
	{
	public:
		virtual XObj* CreateStrObj(const char* data, int size) override;
		virtual bool RegisterPackage(const char* name, PackageCreator creator) override;
		virtual XPackage* CreatePackage(void* pRealObj) override;
		virtual XFunc* CreateFunction(const char* name, U_FUNC func) override;
		virtual XDict* CreateDict() override;
		virtual XObj* ConvertObjFromPointer(void* pObjectPtr) override;
	};
	void CreatXHost();
	void DestoryXHost();
}