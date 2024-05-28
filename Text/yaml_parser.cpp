#include "yaml_parser.h"
#include <iostream>
#include <string.h>

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
			Cleanup();
		}

		bool YamlParser::Init()
		{
			return true;
		}
		bool YamlParser::LoadFromString(char* code, int size)
		{
			if (m_data)
			{
				delete m_data;
			}
			m_data = new char[size];
			memcpy(m_data,code,size);
			m_size = size;
			return true;
		}
		YamlNode* YamlParser::GetParentNode(YamlNode* pCurNode, Status& status)
		{//for sequence
			if (pCurNode == nullptr)
			{
				pCurNode = new YamlNode(nullptr, status);
				pCurNode->SetType(YamlNodeType::Doc);
				status.rootNode = pCurNode;
				return pCurNode;
			}
			else if (pCurNode->Type() == YamlNodeType::Doc)
			{
				return pCurNode;
			}
			if (status.pair_count > 0)
			{
				return pCurNode;
			}
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

		YamlNode* YamlParser::Scan(YamlNode* pCurNode, Status& status)
		{
			//start points ch
			auto start = getPos();
			char ch = getChar();
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
					//while (ch == ' ')
					//{
					//	ch = getChar();
					//}
					start = getPos()-1;
					YamlNode* pMyParentNode = GetParentNode(pCurNode, status);
					//new sequence node
					YamlNode* pNewNode = new YamlNode(start,status);
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
							pCurNode = new YamlNode(start, status);
							pCurNode->SetLeadingInfo(status.LeadingSpaces,
								status.LeadingTabs);
							pCurNode->SetType(YamlNodeType::Doc);
						}
					}
					else
					{

						//treat as Key starts from var:start
						YamlNode* pNewNode = new YamlNode(start, status);
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
					YamlNode* pNewNode = new YamlNode(start, status);
					pNewNode->SetLeadingInfo(status.LeadingSpaces,
						status.LeadingTabs);
					pNewNode->SetType(YamlNodeType::Dict);
					pCurNode->Add(pNewNode);
					pCurNode = pNewNode;
				}
			}
			else if (ch == '{' || ch=='[')
			{
				status.pair_count++;
				start = getPos();
				YamlNode* pMyParentNode = GetParentNode(pCurNode, status);
				YamlNode* pDictNode = new YamlNode(start, status);
				pDictNode->SetParent(pMyParentNode);
				pDictNode->SetLeadingInfo(status.LeadingSpaces,
					status.LeadingTabs);
				pDictNode->SetType(ch == '{' ? YamlNodeType::Dict : YamlNodeType::Sequence);
				pMyParentNode->Add(pDictNode);

				YamlNode* pNewNode = new YamlNode(start, status);
				pNewNode->SetParent(pDictNode);
				pNewNode->SetLeadingInfo(status.LeadingSpaces,
					status.LeadingTabs);
				pDictNode->Add(pNewNode);

				pCurNode = pNewNode;
			}
			else if (ch == ',')
			{
				pCurNode->SetEndPos(start - 1, status.lineNo, status.inQuote);
				YamlNode* pMyParentNode = pCurNode->Parent();
				YamlNode* pNewNode = new YamlNode(start, status);
				pNewNode->SetParent(pMyParentNode);
				pNewNode->SetLeadingInfo(status.LeadingSpaces,
					status.LeadingTabs);
				pMyParentNode->Add(pNewNode);
				pCurNode = pNewNode;
			}
			else if (ch == '}' ||ch ==']')
			{
				status.pair_count--;
				pCurNode->SetEndPos(start, status.lineNo, status.inQuote);
				pCurNode = pCurNode->Parent();
			}
			else if(ch ==':')
			{
				pCurNode->SetEndPos(start, status.lineNo, status.inQuote);
				start = getPos();
				YamlNode* pNewNode = new YamlNode(start, status);
				pCurNode->SetValueNode(pNewNode);
				//still use pCurNode as Current Node
			}
			else if (ch == '\n')
			{//end line
				if (status.pair_count == 0 && pCurNode)
				{
					pCurNode->SetEndPos(start, status.lineNo, status.inQuote);
				}
				status.NewLine = true;
				status.inQuote = false;
				status.LeadingSpaces = 0;
				status.LeadingTabs = 0;
				status.lineNo += 1;
			}
			else if ( (ch == '"' || ch=='\'') && getPrevChar() != '\\')
			{//scan all quoted string
				status.inQuote = true;
				char quote_char = ch;
				char prev_ch = ch;
				ch = getChar();
				while (ch != quote_char || (prev_ch == '\\' && ch == quote_char))//for case \" or \' inside quotes
				{
					prev_ch = ch;
					ch = getChar();
				}
			}
			else if (ch == '#' && getPrevChar()!='\\')//not like \#
			{//scan until meet end of line
				//treat like to end line
				bool curIsClosed = pCurNode?pCurNode->IsClosed():false;
				if (pCurNode && !curIsClosed)
				{
					pCurNode->SetEndPos(start, status.lineNo, status.inQuote);
				}
				char* commment_start = start;
				while (ch != '\n')
				{
					ch = getChar();
				}
				char* commment_end = getPos() - 1;
				if (pCurNode && !curIsClosed)
				{
					pCurNode->SetComment(commment_start, commment_end);
				}
				else
				{//just put into status, then next document will pick it up
					if (status.comment_start == nullptr)
					{//for multiple lines
						status.comment_start = commment_start;
					}
					status.comment_end = commment_end;
				}
				//end line
				status.NewLine = true;
				status.inQuote = false;
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
					YamlNode* pNewNode = new YamlNode(start, status);
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
			{
				//test
				Status s;
				YamlNode* pRootNode = new YamlNode(nullptr, s);
				YamlNode* pNewNode = new YamlNode(nullptr, s);
				pNewNode->SetLeadingInfo(10, 20);
				pNewNode->SetParent(pRootNode);
				pRootNode->Add(pNewNode);
				delete pRootNode;
				delete pNewNode;
			}
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
					//std::cout << (int)e << std::endl;
					break;
				}
				if (pRootNode == nullptr)
				{
					if (status.rootNode)
					{
						pRootNode = status.rootNode;
						status.rootNode = nullptr;
					}
					else
					{
						pRootNode = pCurNode;
					}
				}
			}
			if (pCurNode && !pCurNode->IsClosed())
			{
				pCurNode->SetEndPos(getPos(), status.lineNo, status.inQuote);
			}
			m_root = pRootNode;
			return true;
		}
		void YamlParser::Cleanup()
		{
			if (m_root)
			{
				delete m_root;
				m_root = nullptr;
			}
			if (m_data)
			{
				delete m_data;
			}
		}
	}
}
