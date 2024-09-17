#pragma once

#include "xpackage.h"
#include "xlang.h"
#include "yaml.h"

namespace X 
{
	class YamlWrapper
	{
	public:
		BEGIN_PACKAGE(YamlWrapper)
			APISET().AddFunc<1>("loads", &YamlWrapper::LoadFromString);
			APISET().AddRTFunc<1>("load", &YamlWrapper::LoadFromFile);
			APISET().AddFunc<1>("saves", &YamlWrapper::SaveToYamlString);
			APISET().AddRTFunc<2>("save", &YamlWrapper::SaveToYamlFile);
		END_PACKAGE
	public:
		YamlWrapper()
		{
		}
		~YamlWrapper()
		{

		}
		X::Value LoadFromString(std::string jsonStr);
		X::Value  LoadFromFile(X::XRuntime* rt, X::XObj* pContext, std::string fileName);
		std::string SaveToYamlString(const X::Value& value);
		bool SaveToYamlFile(X::XRuntime* rt, X::XObj* pContext,
			X::Value& value,std::string fileName);
	};
}