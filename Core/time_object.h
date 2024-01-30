#pragma once
#include "xpackage.h"
#include "xlang.h"
#include "utility.h"

namespace X {
	class TimeObject
	{
	public:
		BEGIN_PACKAGE(TimeObject)
			APISET().AddFunc<0>("time", &TimeObject::GetTime);
		END_PACKAGE
	public:
		TimeObject()
		{
		}
		~TimeObject()
		{

		}
		float GetTime()
		{
			auto ll = getCurMicroTimeStamp();
			return (float)ll / 1000000.0f;
		}
	};

}