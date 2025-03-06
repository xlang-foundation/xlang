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

namespace X 
{
	namespace Text
	{
		class YamlParser;
		class YamlNode;
	}
	class YamlNodeWrapper
	{
		Text::YamlNode* m_pNode = nullptr;
		X::Value getNodevalue(Text::YamlNode* pNode);
		X::Value getNodeChildren(Text::YamlNode* pNode);

	public:
		BEGIN_PACKAGE(YamlNodeWrapper)
			APISET().SetAccessor(&YamlNodeWrapper::Access);
			APISET().AddPropL("parent", [](auto* pThis, X::Value v) {},
				[](auto* pThis) {return pThis->get_parent(); });
			APISET().AddPropL("key", [](auto* pThis, X::Value v) {},
				[](auto* pThis) {return pThis->get_key(); });
			APISET().AddPropL("value", [](auto* pThis, X::Value v) {},
				[](auto* pThis) {return pThis->get_value(); });
			APISET().AddPropL("size", [](auto* pThis, X::Value v) {},
				[](auto* pThis) {return pThis->get_size(); });
			APISET().AddPropL("children", [](auto* pThis, X::Value v) {},
				[](auto* pThis) {return pThis->get_children(); });
		END_PACKAGE
	public:
		YamlNodeWrapper()
		{

		}
		YamlNodeWrapper(Text::YamlNode* node)
		{
			m_pNode = node;
		}
		void SetNode(Text::YamlNode* node)
		{
			m_pNode = node;
		}
		X::Value Access(X::Port::vector<X::Value>& IdxAry);
		X::Value get_parent();
		X::Value get_key();
		X::Value get_value();
		X::Value get_size();
		X::Value get_children();
	};
	class YamlWrapper
	{
		Text::YamlParser* m_parser = nullptr;//keep this for content memory, YamlNode use it as ref
	public:
		BEGIN_PACKAGE(YamlWrapper)
			APISET().AddClass<0, YamlNodeWrapper>("Node");
			APISET().AddFunc<1>("loads", &YamlWrapper::LoadFromString);
			APISET().AddRTFunc<1>("load", &YamlWrapper::LoadFromFile);
		END_PACKAGE
	public:
		YamlWrapper()
		{
		}
		~YamlWrapper();
		X::Value LoadFromString(std::string jsonStr);
		X::Value  LoadFromFile(X::XRuntime* rt, X::XObj* pContext, std::string fileName);
	};
}