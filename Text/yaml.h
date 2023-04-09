#pragma once

#include "xpackage.h"
#include "xlang.h"

namespace X 
{
	class YamlWrapper
	{
	public:
		BEGIN_PACKAGE(YamlWrapper)
			APISET().AddFunc<1>("loads", &YamlWrapper::LoadFromString);
			APISET().AddRTFunc<1>("load", &YamlWrapper::LoadFromFile);
		END_PACKAGE
	public:
		YamlWrapper()
		{
		}
		X::Value LoadFromString(std::string jsonStr);
		X::Value  LoadFromFile(X::XRuntime* rt, X::XObj* pContext,std::string fileName);
	};
}