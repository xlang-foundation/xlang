#pragma once

#include "xpackage.h"
#include "xlang.h"
#include "yaml.h"

namespace X 
{
	class Yaml_Wrapper
	{
	public:
		BEGIN_PACKAGE(Yaml_Wrapper)
			APISET().AddFunc<1>("loads", &Yaml_Wrapper::LoadFromString);
			APISET().AddRTFunc<1>("load", &Yaml_Wrapper::LoadFromFile);
			APISET().AddFunc<1>("saves", &Yaml_Wrapper::SaveToYamlString);
			APISET().AddRTFunc<2>("save", &Yaml_Wrapper::SaveToYamlFile);
		END_PACKAGE
	public:
		Yaml_Wrapper()
		{
		}
		~Yaml_Wrapper()
		{

		}
		X::Value LoadFromString(std::string jsonStr);
		X::Value  LoadFromFile(X::XRuntime* rt, X::XObj* pContext, std::string fileName);
		std::string SaveToYamlString(const X::Value& value);
		bool SaveToYamlFile(X::XRuntime* rt, X::XObj* pContext,
			X::Value& value,std::string fileName);
	};
}