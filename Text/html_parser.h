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
			//vars below used for calculate outerHtml, innerHtml
			char* m_nodeStartPtr = nullptr;
			char* m_nodeEndPtr = nullptr;
			char* m_nodeInnerStartPtr = nullptr;
			char* m_nodeInnerEndPtr = nullptr;

			std::string m_class;
			std::unordered_map<std::string, std::string> m_attrs;
			HtmlNode* m_parent = nullptr;
			std::vector<HtmlNode*> m_kids;
			//place content in order, if meet node, insert one space string
			std::vector< std::string> m_contents;
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
			void SetStreamPos(char* pOuterStart, char* pOuterEnd,
				char* pInnerStart, char* pInnterEnd)
			{
				m_nodeStartPtr = (pOuterStart != nullptr) ? pOuterStart : m_nodeStartPtr;
				m_nodeEndPtr = (pOuterEnd != nullptr) ? pOuterEnd : m_nodeEndPtr;
				m_nodeInnerStartPtr = (pInnerStart != nullptr) ? pInnerStart : m_nodeInnerStartPtr;
				m_nodeInnerEndPtr = (pInnterEnd != nullptr) ? pInnterEnd : m_nodeInnerEndPtr;

			}
			bool MatchOneFilter(HtmlNode* pRootNode,HtmlNode* pFilterExpr,
				std::vector<HtmlNode*>& matchedNodes,bool needMatchAll = false);
			bool Query(HtmlNode* pQueryExpr,
				std::vector<HtmlNode*>& matchedNodes,
				bool needMatchAll = false);
			std::string& GetClass() { return m_class; }
			std::string GetInnerText()
			{
				std::string content;
				int nodeIndex = 0;
				int partIndex = 0;
				for (auto& str : m_contents)
				{
					if (str == "")//it is a node
					{
						if (nodeIndex < (int)m_kids.size())
						{
							std::string txt = m_kids[nodeIndex]->GetInnerText();
							if (partIndex == 0)
							{
								content = txt;
							}
							else
							{
								content += " " + txt;
							}
						}
						nodeIndex++;
					}
					else
					{
						if (partIndex == 0)
						{
							content = str;
						}
						else
						{
							content += " " + str;
						}
					}
					partIndex++;
				}
				return content;
			}
			std::string GetContent() 
			{ 
				std::string content;
				int size = (int)m_contents.size();
				if (size == 0)
				{
					return content;
				}
				content = m_contents[0];
				for (int i=1;i<size;i++)
				{
					content += " "+ m_contents[i];
				}
				return content;
			}
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
				m_contents.push_back("");
			}
			void SetContent(std::string& txt)
			{
				m_contents.push_back(txt);
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