#include "yaml_parser.h"
#include <iostream>

//https://yaml.org/spec/1.1/#id857181

namespace X
{
	namespace Text
	{
		YamlParser::YamlParser()
		{
		}

		YamlParser::~YamlParser()
		{

		}

		bool YamlParser::Init()
		{
			return true;
		}
		bool YamlParser::LoadFromString(char* code, int size)
		{
			m_data = code;
			m_size = size;
			return true;
		}
		void YamlParser::Process()
		{
			while (m_bRun)
			{
				auto it0 = m_events.begin();
				auto evt = *it0;
				m_events.erase(it0);
				EventHandler handler = m_Handlers[evt];
				try
				{
					handler(0);
				}
				catch (eventType e) 
				{
					std::cout << (int)e << std::endl;
					Fire(e);
				}
			}
		}
		YamlNode* YamlParser::GetParentNode(YamlNode* pCurNode, Status& status)
		{//for sequence
			YamlNode* pMyParentNode = pCurNode;
			while (pMyParentNode)
			{
				if (status.LeadingSpaces == pMyParentNode->LeadingSpaces())
				{
					pMyParentNode = pMyParentNode->Parent();
					break;
				}
				else if (status.LeadingSpaces < pMyParentNode->LeadingSpaces())
				{
					pMyParentNode = pMyParentNode->Parent();
				}
				else
				{//greater
					break;
				}
			}
			return pMyParentNode;
		}
		YamlNode* YamlParser::GetParentNode2(YamlNode* pCurNode, Status& status)
		{//for sequence
			YamlNode* pMyParentNode = pCurNode;
			//check if pCurNode is a value node
			if (IsValueNode(pCurNode))
			{
				pMyParentNode = pCurNode;
			}
			else
			{
				while (pMyParentNode)
				{
					if (status.LeadingSpaces == pMyParentNode->LeadingSpaces())
					{
						pMyParentNode = pMyParentNode->Parent();
						break;
					}
					else if (status.LeadingSpaces < pMyParentNode->LeadingSpaces())
					{
						pMyParentNode = pMyParentNode->Parent();
						if (pMyParentNode && IsValueNode(pMyParentNode))
						{
							pMyParentNode = pMyParentNode->Parent();
						}
					}
					else
					{//greater
						break;
					}
				}
			}
			return pMyParentNode;
		}
		bool YamlParser::IsValueNode(YamlNode* pNode)
		{
			return (pNode->Parent() &&
				pNode->Parent()->ValueNode() == pNode);
		}
		YamlNode* YamlParser::GetParentNode3(YamlNode* pCurNode, Status& status)
		{
			YamlNode* pMyParentNode = pCurNode;
			if (IsValueNode(pMyParentNode))
			{
				//compair wtih the main key part's LeadingSpace
				YamlNode* pKeyNode = pMyParentNode->Parent();
				if (status.LeadingSpaces == pKeyNode->LeadingSpaces())
				{
					pMyParentNode = pKeyNode->Parent();
					return pMyParentNode;
				}
				else if(status.LeadingSpaces > pKeyNode->LeadingSpaces())
				{
					return pCurNode;
				}
			}
			if (status.LeadingSpaces <= pMyParentNode->LeadingSpaces())
			{
				pMyParentNode = pMyParentNode->Parent();
				if (IsValueNode(pMyParentNode))
				{
					pMyParentNode = pMyParentNode->Parent();
				}
				while (pMyParentNode &&
					status.LeadingSpaces <= pMyParentNode->LeadingSpaces())
				{
					pMyParentNode = pMyParentNode->Parent();
					if (pMyParentNode && IsValueNode(pMyParentNode))
					{
						pMyParentNode = pMyParentNode->Parent();
					}
				}
			}
			return pMyParentNode;
		}
		YamlNode* YamlParser::Scan(YamlNode* pCurNode, Status& status)
		{
			char ch = getChar();
			auto start = getPos();
			if (status.NewLine)
			{//Count leading space
				while (ch == ' ' || ch == '\t')
				{
					if (ch == ' ')
					{
						status.LeadingSpaces += 1;
					}
					else
					{
						status.LeadingTabs += 1;
					}
					ch = getChar();
				}
			}
			if (ch == '-' && status.NewLine)
			{
				ch = getChar();
				if (ch == ' ')
				{
					start = getPos();
					YamlNode* pMyParentNode = GetParentNode(pCurNode, status);
					//new sequence node
					YamlNode* pNewNode = new YamlNode(start,status.lineNo);
					pNewNode->SetLeadingInfo(status.LeadingSpaces,
						status.LeadingTabs);
					pNewNode->SetType(YamlNodeType::Sequence);
					pNewNode->SetParent(pMyParentNode);
					pMyParentNode->Add(pNewNode);
					pCurNode = pNewNode;
				}
				else if (ch == '-')
				{
					ch = getChar();
					if (ch == '-')
					{
						if (pCurNode
							&& pCurNode->Type() == YamlNodeType::Doc
							&& pCurNode->IsNullStartPos())
						{
							pCurNode->SetStartPos(start,status.lineNo);
						}
						else
						{//new document
							pCurNode = new YamlNode(start, status.lineNo);
							pCurNode->SetLeadingInfo(status.LeadingSpaces,
								status.LeadingTabs);
							pCurNode->SetType(YamlNodeType::Doc);
						}
					}
					else
					{
						//treat as Key starts from var:start
						YamlNode* pNewNode = new YamlNode(start, status.lineNo);
						pNewNode->SetLeadingInfo(status.LeadingSpaces,
							status.LeadingTabs);
						pNewNode->SetType(YamlNodeType::Dict);
						pCurNode->Add(pNewNode);
						pCurNode = pNewNode;
					}
				}
				else
				{
					//treat as Key starts from var:start
					YamlNode* pNewNode = new YamlNode(start, status.lineNo);
					pNewNode->SetLeadingInfo(status.LeadingSpaces,
						status.LeadingTabs);
					pNewNode->SetType(YamlNodeType::Dict);
					pCurNode->Add(pNewNode);
					pCurNode = pNewNode;
				}
			}
			else if(ch ==':')
			{
				pCurNode->SetEndPos(start-1, status.lineNo);
				start = getPos();
				YamlNode* pNewNode = new YamlNode(start, status.lineNo);
				pCurNode->SetValueNode(pNewNode);
				//still use pCurNode as Current Node
				//pCurNode = pNewNode;
			}
			else if (ch == '\n')
			{//end line
				pCurNode->SetEndPos(start-1, status.lineNo);
				status.NewLine = true;
				status.LeadingSpaces = 0;
				status.LeadingTabs = 0;
				status.lineNo += 1;
			}
			else
			{
				//Reset line
				status.NewLine = false;
				if (pCurNode->IsClosed())
				{
					YamlNode* pMyParentNode = GetParentNode(pCurNode,status);
					YamlNode* pNewNode = new YamlNode(start, status.lineNo);
					pNewNode->SetParent(pMyParentNode);
					pNewNode->SetLeadingInfo(status.LeadingSpaces,
						status.LeadingTabs);
					pMyParentNode->Add(pNewNode);
					pCurNode = pNewNode;
				}
			}
			return pCurNode;
		}
		bool YamlParser::Parse()
		{
			m_cur = m_data;
			m_last = m_data + m_size;
			m_bRun = true;
			Status status;
			status.NewLine = true;
			YamlNode* pRootNode = nullptr;
			YamlNode* pCurNode = nullptr;
			while (m_bRun)
			{
				try
				{
					pCurNode = Scan(pCurNode, status);
				}
				catch (eventType e)
				{
					std::cout << (int)e << std::endl;
					break;
				}
				if (pRootNode == nullptr)
				{
					pRootNode = pCurNode;
				}
			}
			return true;
		}
		bool YamlParser::Parse3()
		{
			On(eventType::Start, [this](int x)
			{
				getChar();
			});
			On(eventType::End, [this](int x)
			{
					m_bRun = false;
			});
			//init
			m_cur = m_data;
			//m_last = m_data + m_size;
			m_bRun = true;
			Fire(eventType::Start);
			Process();
			return true;
		}
		bool YamlParser::Parse2()
		{
			struct statusInfo
			{
				
			};
			char* start = m_data;
			char* end = m_data + m_size;
			while (start<end)
			{
				char ch = *start++;
				//feed space at line begin
				int space_cnt = 0;
				while (ch == ' ' && start<end)
				{
					space_cnt++;
					ch = *start++;
				}
				if (ch == '-')
				{
					if (space_cnt == 0)
					{
						if (start < (end - 2) && *start =='-' && *(start+1) =='-')
						{
							//document start
							start += 2;
							continue;
						}
					}
					if (start < end && *start == ' ')
					{
						//enter sequence
						start++;
						continue;
					}
				}
				if (ch == ':')
				{
					if (start < end && *start == ' ')
					{
						//meet common case
						start++;
						//process chars before

						continue;
					}
				}
				if (ch == '"' || ch == '\'')
				{

				}
				if (ch == '#')
				{

				}
				if (ch == '\n')
				{

				}
			}
			return true;
		}
	}
}
