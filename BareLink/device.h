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

				END_PACKAGE
			Device(std::string deviceId);
			~Device();
		};
	}
}