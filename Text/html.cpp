#include "html.h"
#include "Hosting.h"
#include "utility.h"
#include "port.h"
#include "html_parser.h"

namespace X
{
	X::Value HtmlWrapper::LoadFromString(std::string htmlStr)
	{
		Text::Html html;
		html.Init();
		Text::HtmlNode* pRootNode = nullptr;
		html.LoadFromString((char*)htmlStr.c_str(), (int)htmlStr.size(),
			&pRootNode);
		X::XPackageValue<HtmlNodeWrapper> valNode;
		(*valNode).SetNode(pRootNode);
		return valNode;
	}
	X::Value  HtmlWrapper::LoadFromFile(X::XRuntime* rt, X::XObj* pContext,
		std::string fileName)
	{
		if (!IsAbsPath(fileName))
		{
			X::XlangRuntime* pRt = (X::XlangRuntime*)rt;
			std::string curPath = pRt->M()->GetModulePath();
			fileName = curPath + Path_Sep_S + fileName;
		}
		X::Value retValue;
		std::string htmlStr;
		bool bOK = LoadStringFromFile(fileName, htmlStr);
		if (!bOK)
		{
			retValue = X::Value(false);
		}
		Text::Html html;
		html.Init();
		Text::HtmlNode* pRootNode = nullptr;
		html.LoadFromString((char*)htmlStr.c_str(), (int)htmlStr.size(),
			&pRootNode);
		X::XPackageValue<HtmlNodeWrapper> valNode;
		(*valNode).SetNode(pRootNode);
		return valNode;
	}
	X::Value HtmlNodeWrapper::GetClass()
	{
		if (m_pNode == nullptr)
		{
			return X::Value();
		}
		else
		{
			return X::Value(m_pNode->GetClass());
		}
	}
	X::Value HtmlNodeWrapper::GetContent()
	{
		if (m_pNode == nullptr)
		{
			return X::Value();
		}
		else
		{
			std::string cnt = m_pNode->GetContent();
			return X::Value(cnt);
		}
	}
	X::Value HtmlNodeWrapper::GetInnerText()
	{
		if (m_pNode == nullptr)
		{
			return X::Value();
		}
		else
		{
			std::string cnt = m_pNode->GetInnerText();
			return X::Value(cnt);
		}
	}
	X::Value HtmlNodeWrapper::GetParent()
	{
		if (m_pNode == nullptr)
		{
			return X::Value();
		}
		X::XPackageValue<HtmlNodeWrapper> valNode;
		(*valNode).SetNode(m_pNode->GetParent());
		return valNode;
	}
	X::Value HtmlNodeWrapper::GetKids()
	{
		X::List list;
		auto& nodes = m_pNode->GetKids();
		for (auto* node : nodes)
		{
			X::XPackageValue<HtmlNodeWrapper> valNode;
			(*valNode).SetNode(node);
			list += valNode;
		}
		return list;
	}
	X::Value HtmlNodeWrapper::GetAttrs()
	{
		X::Dict dict;
		auto& attrs = m_pNode->GetAttrs();
		for (auto& it : attrs)
		{
			std::string strKey = it.first;
			X::Value key(strKey);
			X::Value val(it.second);
			dict->Set(key,val);
		}
		return dict;
	}
	X::Value HtmlNodeWrapper::Query(std::string queryString)
	{
		if (m_pNode == nullptr)
		{
			return X::Value(false);
		}
		Text::Html html;
		html.Init();
		Text::HtmlNode* pFilterExprNode = nullptr;
		html.LoadFromString((char*)queryString.c_str(), (int)queryString.size(),
			&pFilterExprNode);
		std::vector<Text::HtmlNode*> matchedNodes;
		bool bOK = m_pNode->Query(pFilterExprNode, matchedNodes,true);
		if (bOK)
		{
			X::List listNodes;
			for (auto* pNode : matchedNodes)
			{
				X::XPackageValue<HtmlNodeWrapper> valNode;
				(*valNode).SetNode(pNode);
				listNodes += valNode;
			}
			return listNodes;
		}
		else
		{
			return X::Value(bOK);
		}
	}
}