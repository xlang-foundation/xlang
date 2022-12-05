#include "html_parser.h"
#include "lex.h"
#include <iostream>
#include <stack>

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
			bool bOK =  Parse(ppRootNode);
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
					"source","track","wbr"};
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
						ns.pNode->SetAttr(left.text,right.text);
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
						if (pCurNode && operands.size()>0)
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
							for (int i=(int)ns.contents.size()-1;i>=0;i--)
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
								short idx2 = mToken->UntilGet(tag,one);
								if (idx2 == TokenStr)
								{
									String s2 = one.id;
									std::string txt2(s2.s, s2.size- (int)tag.size());
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

				std::string txt(s.s, s.size);
				std::cout << "token{" << txt << "},idx:" << idx << ",line:"
					<< one.lineStart << ",pos:" << one.charPos << std::endl;
			}
			*ppRootNode = pRootNode;
			return true;
		}
	}
}
