/*
Copyright (C) 2024 The XLang Foundation
Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

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