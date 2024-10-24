﻿/*
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

namespace X 
{
	class JsonWrapper
	{
	public:
		BEGIN_PACKAGE(JsonWrapper)
			APISET().AddFunc<1>("loads", &JsonWrapper::LoadFromString);
			APISET().AddRTFunc<1>("load", &JsonWrapper::LoadFromFile);
			APISET().AddFunc<1>("saves", &JsonWrapper::SaveToString);
			END_PACKAGE
	public:
		JsonWrapper()
		{
		}
		std::string SaveToString(X::Value v)
		{
			return v.ToString(true);
		}
		X::Value LoadFromString(std::string jsonStr);
		X::Value  LoadFromFile(X::XRuntime* rt, X::XObj* pContext,std::string fileName);
	};
}