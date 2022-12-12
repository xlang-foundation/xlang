#include "html_parser.h"
#include "lex.h"
#include <iostream>
#include <stack>
#include "port.h"

#ifndef strcasecmp
#define strcasecmp _stricmp
#endif // strcasecmp

namespace X
{
	namespace Text
	{
		enum class htmlsymbol
		{
			TagBeginLeft,
			TagBeginRight,
			TagEnd,
			TagEndWithName,
			DocTagBegin,
			Assign,
			Comment_Begin,
			Comment_Begin2,
		};
		std::vector<OpInfo> Html::OPList =
		{
			{(int)htmlsymbol::TagBeginLeft,"<"},
			{(int)htmlsymbol::TagBeginRight,">"},
			{(int)htmlsymbol::TagEnd,"/>"},
			{(int)htmlsymbol::TagEndWithName,"</"},
			{(int)htmlsymbol::DocTagBegin,"<!"},
			{(int)htmlsymbol::Assign,"="},
			{(int)htmlsymbol::Comment_Begin,"<!--"},
			{(int)htmlsymbol::Comment_Begin2,"<!--[if"},
		};
		struct Operand
		{
			std::string text;
		};
		struct Operator
		{
			htmlsymbol op;
			std::string name;
		};

		std::vector<short> Html::kwTree;
		std::vector<OpAction> Html::OpActions;

		Html::Html()
		{
		}

		Html::~Html()
		{
			if (mToken)
			{
				delete mToken;
			}
		}

		bool Html::Init()
		{
			Lex<OpInfo, OpAction>().MakeLexTree(
				OPList, kwTree, OpActions);
			mToken = new Token(&kwTree[0]);
			mToken->set_ops("\t\r\n></=!");
			mToken->SetSkipHash(true);
			return true;
		}
		bool Html::LoadFromString(char* code, int size, HtmlNode** ppRootNode)
		{
			mToken->SetStream(code, size);
			bool bOK = Parse(ppRootNode);
			return bOK;
		}
		bool Html::Parse(HtmlNode** ppRootNode)
		{
			std::stack<Operand> operands;
			std::stack<Operator> ops;
			struct NodeStatus
			{
				HtmlNode* pNode = nullptr;
				bool isCloseTag = false;
				std::string closeTag;
				std::vector<std::string> contents;
			};
			auto is_empty_tag = [](std::string& tagName)
			{
				static std::vector<std::string> tags =
				{ "area","base","br","col","embed","hr","img",
					"input","keygen","link","meta","param",
					"source","track","wbr" };
				bool bFind = false;
				for (auto& s : tags)
				{
					if (s == tagName)
					{
						bFind = true;
						break;
					}
				}
				return bFind;

			};
			auto set_content = [&](HtmlNode* pCurNode)
			{
				std::string txt;
				while (operands.size() > 0)
				{
					std::string& t = operands.top().text;
					if (txt.empty())
					{
						txt = t;
					}
					else
					{
						txt = t + ' ' + txt;
					}
					operands.pop();
				}
				pCurNode->SetContent(txt);
			};
			auto do_TagBeginRight = [&](htmlsymbol sym)
			{
				NodeStatus ns;
				while (!ops.empty())
				{
					auto opInfo = ops.top();
					ops.pop();
					switch (opInfo.op)
					{
					case htmlsymbol::TagBeginLeft:
						if (ns.pNode == nullptr)
						{
							ns.pNode = new HtmlNode();
						}
						ns.pNode->SetClass(operands.top().text);
						operands.pop();
						break;
					case htmlsymbol::TagEndWithName:
						ns.isCloseTag = true;
						ns.closeTag = operands.top().text;
						operands.pop();
						while (!operands.empty())
						{
							ns.contents.push_back(operands.top().text);
							operands.pop();
						}
						break;
					case htmlsymbol::Assign:
					{
						if (ns.pNode == nullptr)
						{
							ns.pNode = new HtmlNode();
						}
						auto right = operands.top();
						operands.pop();
						auto left = operands.top();
						operands.pop();
						ns.pNode->SetAttr(left.text, right.text);
					}
					break;
					default:
						break;
					}
				}
				return ns;
			};
			HtmlNode* pRootNode = nullptr;
			HtmlNode* pCurNode = nullptr;
			while (true)
			{
				String s;
				int leadingSpaceCnt = 0;
				OneToken one;
				short idx = mToken->Get(one);
				int startLine = one.lineStart;
				s = one.id;
				if (idx == TokenEOS)
				{
					break;
				}
				if (idx >= 0)
				{
					htmlsymbol sym = (htmlsymbol)idx;
					std::cout << (int)sym;
					switch (sym)
					{
					case htmlsymbol::Comment_Begin:
					case htmlsymbol::Comment_Begin2:
					{
						mToken->SetSkipQuote(true);
						std::string tag("-->");
						short idx2 = mToken->UntilGet(tag, one);
						if (idx2 == TokenStr)
						{
							HtmlNode* pNode = new HtmlNode();
							std::string comment("comment");
							pNode->SetClass(comment);
							String s2 = one.id;
							std::string txt2(s2.s, s2.size - (int)tag.size());
							pNode->SetContent(txt2);
							if (pRootNode == nullptr)
							{
								pRootNode = new HtmlNode();
								pCurNode = pRootNode;
								pCurNode->AddChild(pNode);
							}
							else
							{
								pCurNode->AddChild(pNode);
							}
						}
					}
					break;
					case htmlsymbol::TagBeginLeft:
						mToken->SetSkipQuote(false);
						if (pCurNode && operands.size() > 0)
						{
							set_content(pCurNode);
						}
						ops.push(Operator{ sym });
						break;
					case htmlsymbol::TagBeginRight:
					{
						mToken->SetSkipQuote(true);
						auto ns = do_TagBeginRight(sym);
						if (ns.isCloseTag)
						{
							for (int i = (int)ns.contents.size() - 1; i >= 0; i--)
							{
								pCurNode->SetContent(ns.contents[i]);
							}
							pCurNode = pCurNode->Parent();
						}
						else
						{
							if (ns.pNode && is_empty_tag(ns.pNode->GetClass()))
							{
								if (pRootNode == nullptr)
								{
									pRootNode = ns.pNode;
									pCurNode = ns.pNode;
								}
								else
								{
									pCurNode->AddChild(ns.pNode);
								}
							}
							else if (ns.pNode && ns.pNode->GetClass() == "script")
							{
								std::string tag("</script>");
								short idx2 = mToken->UntilGet(tag, one);
								if (idx2 == TokenStr)
								{
									String s2 = one.id;
									std::string txt2(s2.s, s2.size - (int)tag.size());
									ns.pNode->SetContent(txt2);
									if (pRootNode == nullptr)
									{
										pRootNode = ns.pNode;
										pCurNode = ns.pNode;
									}
									else
									{
										pCurNode->AddChild(ns.pNode);
									}
								}
							}
							else if (ns.pNode && ns.pNode->GetClass() == "style")
							{
								std::string tag("</style>");
								short idx2 = mToken->UntilGet(tag, one);
								if (idx2 == TokenStr)
								{
									String s2 = one.id;
									std::string txt2(s2.s, s2.size - (int)tag.size());
									ns.pNode->SetContent(txt2);
									if (pRootNode == nullptr)
									{
										pRootNode = ns.pNode;
										pCurNode = ns.pNode;
									}
									else
									{
										pCurNode->AddChild(ns.pNode);
									}
								}
							}
							else if (pRootNode == nullptr)//pCurNode is also null
							{//first node as root
								pRootNode = ns.pNode;
								pCurNode = ns.pNode;
							}
							else
							{
								pCurNode->AddChild(ns.pNode);
								//new node set as Current Node to accept kids under
								pCurNode = ns.pNode;
							}
						}
					}
					break;
					case htmlsymbol::TagEnd:
					{
						mToken->SetSkipQuote(true);
						auto ns = do_TagBeginRight(sym);
						if (ns.isCloseTag)
						{
							pCurNode = pCurNode->Parent();
						}
						else
						{
							pCurNode->AddChild(ns.pNode);
							//don't change Current Node,still as parent
						}
					}
					break;
					case htmlsymbol::TagEndWithName:
						mToken->SetSkipQuote(false);
						ops.push(Operator{ sym });
						break;
					case htmlsymbol::DocTagBegin:
						break;
					case htmlsymbol::Assign:
						ops.push(Operator{ sym });
						break;
					default:
						break;
					}
				}
				else
				{
					std::string txt(s.s, s.size);
					operands.push(Operand{ txt });
				}
#if 0
				std::string txt(s.s, s.size);
				std::cout << "token{" << txt << "},idx:" << idx << ",line:"
					<< one.lineStart << ",pos:" << one.charPos << std::endl;
#endif
			}
			*ppRootNode = pRootNode;
			return true;
		}
		bool HtmlNode::NodeMatch(HtmlNode* pCurNode, HtmlNode* pExprNode)
		{
			if (pCurNode->m_class != pExprNode->m_class)
			{
				return false;
			}
			bool bRet = true;
			for (auto& it : pExprNode->m_attrs)
			{
				if (it.first.starts_with("${"))
				{
					continue;
				}
				//find item with key
				auto it2 = pCurNode->m_attrs.find(it.first);
				if (it2 != pCurNode->m_attrs.end())
				{
					std::string& valToMatch = it.second;
					if (valToMatch.starts_with("regex:"))
					{//regular expression match
						//todo:
					}
					else
					{
						if (valToMatch != it2->second)
						{
							bRet = false;
							break;
						}
					}
				}
				else
				{
					bRet = false;
					break;
				}
			}
			return bRet;
		}
		bool HtmlNode::DoMatch(HtmlNode* pCurNode, HtmlNode* pExprNode)
		{
			bool bMatched = false;
			if (NodeMatch(pCurNode, pExprNode))
			{
				int levelOfChildRequired = 0;//any descendant
				if (pExprNode->m_kids.size() > 0)
				{
					auto* pFirstExprNode = pExprNode->m_kids[0];
					std::string strCombinator;
					if (pFirstExprNode->GetAttr("${child_combinator}", strCombinator))
					{
						if (strCombinator == "*")
						{
							levelOfChildRequired = -1;
						}
						else
						{
							SCANF(strCombinator.c_str(), "%d", &levelOfChildRequired);
						}
					}
				}
				bMatched = MatchExprKidsWithFilterSubItems(
					pCurNode->m_kids, pExprNode);

			}
			return bMatched;
		}
		//in filter if logical op is or, and no ${node_index} specified
		//means, start matching from first child
		//if it is and, means following the previous matched child node
		//${node_index} define here
		//if Negative value means offset such as -1,-2, means the kidIndex-number
		//if +num, means offset of positive,
		//if no sign there, means absoluate position in  kids array
		//if it is 'last', means last kid

		bool HtmlNode::MatchExprKidsWithFilterSubItems(
			std::vector<HtmlNode*>& kids,
			HtmlNode* pExprParentNode)
		{
			int kidCnt = (int)kids.size();
			int filterKidCnt = (int)pExprParentNode->m_kids.size();
			if (kidCnt ==0 && filterKidCnt == 0)
			{
				return true;//both empty, return true
			}
			else if (kidCnt == 0)
			{
				return false;//filterKidCnt is not 0
			}
			int kidIndex = 0;
			int filtetKidIndex = 0;
			bool bMatchAll = true;

			while (filtetKidIndex < filterKidCnt)
			{
				auto* pCurFilterKid = pExprParentNode->m_kids[filtetKidIndex];
				bool isLogical_Or = pCurFilterKid->MatchAttr("${logical}", "or");
				std::string strIndex;
				if (pCurFilterKid->GetAttr("${node_index}", strIndex))
				{
					if (strIndex.size() > 0)
					{
						if (strIndex[0] == '+')
						{
							int indexOffset = 0;
							SCANF(strIndex.c_str()+1, "%d", &indexOffset);
							kidIndex += indexOffset;
							if (kidIndex >= kidCnt)
							{
								kidIndex = kidCnt - 1;
							}
						}
						else if (strIndex[0] == '-')
						{
							int indexOffset = 0;
							SCANF(strIndex.c_str() + 1, "%d", &indexOffset);
							kidIndex -= indexOffset;
							if (kidIndex < 0)
							{
								kidIndex = 0;
							}
						}
						else if (strIndex == "last")
						{
							kidIndex = kidCnt - 1;
						}
						else
						{
							int index = 0;
							SCANF(strIndex.c_str(), "%d", &index);
							kidIndex = index;
							if (kidIndex < 0)
							{
								kidIndex = 0;
							}
							else if (kidIndex >= kidCnt)
							{
								kidIndex = kidCnt - 1;
							}
						}
					}
				}
				else
				{
					if (isLogical_Or)
					{
						//reset to first kid
						kidIndex = 0;
					}
					else if (filtetKidIndex > 0)
					{//if it is not first filter node, move matching node also
						//if it is and ( default it is)
						//move to next kid
						kidIndex++;
					}
				}
				if (isLogical_Or && bMatchAll)
				{//it is or but bMatchAll is aready true, 
				//don't need to calculate DoMatch
					filtetKidIndex++;
					continue;
				}
				else if (!isLogical_Or && (!bMatchAll))
				{// it is and,but bMatchAll already false
				//don't need to calculate DoMatch
					filtetKidIndex++;
					continue;
				}
				else
				{
					auto* pCurKid = kids[kidIndex];
					bool bMatch = DoMatch(pCurKid, pCurFilterKid);
					filtetKidIndex++;
					if (isLogical_Or)
					{
						bMatchAll = bMatchAll || bMatch;
					}
					else
					{
						bMatchAll = bMatchAll && bMatch;
					}
				}
			}
			return bMatchAll;
		}
		bool HtmlNode::MatchExprSubItems(HtmlNode* pCurNode, HtmlNode* pExprParentNode)
		{
			bool bMatchAll = true;
			bool bFirst = true;
			for (auto* pFilterItem : pExprParentNode->m_kids)
			{
				bool IsAnd = true;
				if (!bFirst)
				{//start from second item, need to check logical operator
					if (pFilterItem->MatchAttr("${logical}", "and"))
					{
						IsAnd = true;
					}
					else if (pFilterItem->MatchAttr("${logical}", "or"))
					{
						IsAnd = false;
					}
					if (!IsAnd && bMatchAll)
					{//it is or but bMatchAll is aready true, 
					//don't need to calculate DoMatch
						continue;
					}
					else if (IsAnd && (!bMatchAll))
					{// it is and,but bMatchAll already false
					//don't need to calculate DoMatch
						continue;
					}
				}
				bool bMatch = DoMatch(pCurNode, pFilterItem);
				if (bFirst)
				{
					bMatchAll = bMatch;
					bFirst = false;
					continue;
				}
				if (IsAnd)
				{
					bMatchAll = bMatchAll && bMatch;
				}
				else
				{
					bMatchAll = bMatchAll || bMatch;
				}
			}
			return bMatchAll;
		}
		bool HtmlNode::MatchOneFilter(HtmlNode* pRootNode, HtmlNode* pFilterExpr,
			std::vector<HtmlNode*>& matchedNodes, bool needMatchAll)
		{
			if (pFilterExpr->m_kids.size() == 0)
			{
				return false;
			}
			std::vector<HtmlNode*> nodes;
			nodes.push_back(pRootNode);
			bool bMatched =  MatchExprKidsWithFilterSubItems(
				nodes,pFilterExpr);
			if (bMatched)
			{
				matchedNodes.push_back(pRootNode);
			}
			if(needMatchAll || !bMatched)
			{
				//look into children and descendants
				for (auto* pKid : pRootNode->m_kids)
				{
					bMatched |= MatchOneFilter(pKid, pFilterExpr,
						matchedNodes, needMatchAll);
				}
			}
			return bMatched;
		}
		bool HtmlNode::Query(HtmlNode* pQueryExpr,
			std::vector<HtmlNode*>& matchedNodes,
			bool needMatchAll)
		{
			bool bOK = false;
			for (auto* pSubNode : pQueryExpr->m_kids)
			{
				if (strcasecmp(pSubNode->m_class.c_str(),"match")==0)
				{
					bOK |= MatchOneFilter(this,pSubNode, matchedNodes, needMatchAll);
				}
			}
			return bOK;
		}
	}
}
