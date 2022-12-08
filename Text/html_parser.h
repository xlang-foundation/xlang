#pragma once
#include "token.h"
#include "def.h"
#include <vector>
#include <unordered_map>
#include <string>

namespace X
{
	namespace Text
	{
		class HtmlNode
		{
			std::string m_class;
			std::unordered_map<std::string, std::string> m_attrs;
			HtmlNode* m_parent = nullptr;
			std::vector<HtmlNode*> m_kids;
			std::string m_content;//only for text content embeded into an object
			bool NodeMatch(HtmlNode* pCurNode, HtmlNode* pExprNode);
			bool DoMatch(HtmlNode* pCurNode, HtmlNode* pExprNode);
			bool MatchExprKidsWithFilterSubItems(
				std::vector<HtmlNode*>& kids, HtmlNode* pExprParentNode);
			bool MatchExprSubItems(HtmlNode* pCurNode, HtmlNode* pExprParentNode);
			bool MatchAttr(std::string key, std::string val)
			{
				auto it = m_attrs.find(key);
				return (it != m_attrs.end() && it->second == val);
			}
			bool GetAttr(std::string key, std::string& val)
			{
				auto it = m_attrs.find(key);
				if (it != m_attrs.end())
				{
					val = it->second;
					return true;
				}
				else
				{
					return false;
				}
			}
		public:
			~HtmlNode()
			{
				for (auto* p : m_kids)
				{
					delete p;
				}
			}
			bool MatchOneFilter(HtmlNode* pRootNode,HtmlNode* pFilterExpr);
			bool Query(HtmlNode* pQueryExpr);
			std::string& GetClass() { return m_class; }
			std::string& GetContent() { return m_content; }
			std::unordered_map<std::string, std::string>& GetAttrs()
			{
				return m_attrs;
			}
			HtmlNode* GetParent() { return m_parent; }
			std::vector<HtmlNode*>& GetKids() { return m_kids; }
			void SetAttr(std::string& key, std::string& val)
			{
				m_attrs.emplace(std::make_pair(key, val));
			}
			void SetClass(std::string& clsName)
			{
				m_class = clsName;
			}
			void AddChild(HtmlNode* pChildNode)
			{
				pChildNode->m_parent = this;
				m_kids.push_back(pChildNode);
			}
			void SetContent(std::string& txt)
			{
				if (m_content.empty())
				{
					m_content = txt;
				}
				else
				{
					m_content = m_content+' '+txt;
				}
			}
			HtmlNode* Parent() { return m_parent; }
		};
		class Html
		{
			Token* mToken = nullptr;
			static std::vector<OpInfo> OPList;
			static std::vector<short> kwTree;
			static std::vector<OpAction> OpActions;
			bool Parse(HtmlNode** ppRootNode);
		public:
			Html();
			~Html();
			bool Init();
			bool LoadFromString(char* code, int size,HtmlNode** ppRootNode);
		};
	}
}