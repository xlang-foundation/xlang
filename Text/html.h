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