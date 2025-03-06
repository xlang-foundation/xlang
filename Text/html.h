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
	namespace Text{
		class HtmlNode;
	}
	class HtmlNodeWrapper
	{
		Text::HtmlNode* m_pNode = nullptr;
	public:
		BEGIN_PACKAGE(HtmlNodeWrapper)
			APISET().AddPropL("class", [](auto* pThis, X::Value v) {},
				[](auto* pThis) {return pThis->GetClass(); });
			APISET().AddPropL("content", [](auto* pThis, X::Value v) {},
				[](auto* pThis) {return pThis->GetContent(); });
			APISET().AddPropL("innerText", [](auto* pThis, X::Value v) {},
				[](auto* pThis) {return pThis->GetInnerText(); });
			APISET().AddPropL("parent", [](auto* pThis, X::Value v) {},
				[](auto* pThis) {return pThis->GetParent(); });
			APISET().AddPropL("kids", [](auto* pThis, X::Value v) {},
				[](auto* pThis) {return pThis->GetKids(); });
			APISET().AddPropL("attrs", [](auto* pThis, X::Value v) {},
				[](auto* pThis) {return pThis->GetAttrs(); });
			APISET().AddFunc<1>("query", &HtmlNodeWrapper::Query);
		END_PACKAGE
	public:
		HtmlNodeWrapper()
		{
		}
		void SetNode(Text::HtmlNode* p)
		{
			m_pNode = p;
		}
		X::Value GetClass();
		X::Value GetContent();
		X::Value GetInnerText();
		X::Value GetParent();
		X::Value GetKids();
		X::Value GetAttrs();
		X::Value Query(std::string queryString);
	};
	class HtmlWrapper
	{
	public:
		BEGIN_PACKAGE(HtmlWrapper)
		APISET().AddClass<0, HtmlNodeWrapper>("Node");
		APISET().AddFunc<1>("loads", &HtmlWrapper::LoadFromString);
		APISET().AddRTFunc<1>("load", &HtmlWrapper::LoadFromFile);
		END_PACKAGE
	public:
		HtmlWrapper()
		{
		}
		X::Value LoadFromString(std::string htmlStr);
		X::Value  LoadFromFile(X::XRuntime* rt, X::XObj* pContext, std::string fileName);
	};
}