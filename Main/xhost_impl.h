#pragma once
#include "xhost.h"

namespace X
{
	class XHost_Impl :
		public XHost
	{
	public:
		virtual XObj* CreateStrObj(const char* data, int size) override;
	};
	void CreatXHost();
	void DestoryXHost();
}