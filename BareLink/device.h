#pragma once
#include "xpackage.h"
#include "xlang.h"

namespace X
{
	namespace BareLink
	{
		class Device
		{
		public:
			BEGIN_PACKAGE(Device)
				APISET().AddFunc<0>("Connect", &Device::Connect);
				APISET().AddFunc<0>("Disconnect", &Device::Disconnect);
			END_PACKAGE
			Device(std::string deviceId);
			bool Connect();
			bool Disconnect();
			~Device();
		};
	}
}