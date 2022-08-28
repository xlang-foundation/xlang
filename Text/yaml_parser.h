#pragma once

#include <functional>
#include <unordered_map>
#include <vector>
namespace X
{
	namespace Text
	{
		enum class YamlNodeType
		{
			Node,
			Doc,
			Sequence,
			Dict
		};
		class YamlNode;
		struct Status
		{
			YamlNode* rootNode = nullptr;
			bool NewLine = false;
			int lineNo = 1;
			int LeadingSpaces = 0;
			int LeadingTabs = 0;
			int pair_count = 0;
			//for comment before document start
			char* comment_start = nullptr;
			char* comment_end = nullptr;
		};
		class YamlNode
		{
		protected:
			char* m_start_pos =nullptr;
			char* m_end_pos = nullptr;
			int m_startLineNo = 0;
			int m_endLineNo = 0;
			std::vector<YamlNode*> m_children;
			YamlNode* m_parent = nullptr;
			YamlNode* m_valueNode = nullptr;//for dict node
			YamlNodeType m_type = YamlNodeType::Node;

			int m_leadingSpaces = 0;
			int m_leadingTabs = 0;

			//for comment
			char* m_comment_start = nullptr;
			char* m_comment_end = nullptr;
		public:
			YamlNode(char* startPos,Status& s)
			{
				m_start_pos = startPos;
				m_startLineNo = s.lineNo;
				if (s.comment_start != nullptr &&
					s.comment_end != nullptr)
				{
					SetComment(s.comment_start, s.comment_end);
					s.comment_start = s.comment_end = nullptr;
				}
			}
			~YamlNode()
			{
				for (auto& p : m_children)
				{
					delete p;
				}
				if (m_valueNode)
				{
					delete m_valueNode;
				}
			}
			void SetStartPos(char* p,int lineNo)
			{
				m_start_pos = p;
				m_startLineNo = lineNo;
			}
			void SetEndPos(char* p, int lineNo)
			{
				if(m_valueNode)
				{ 
					m_valueNode->SetEndPos(p, lineNo);
				}
				else
				{
					m_end_pos = p;
					m_endLineNo = lineNo;
				}
			}
			void SetLeadingInfo(int spaces, int tabs)
			{
				m_leadingSpaces = spaces;
				m_leadingTabs = tabs;
			}
			void SetComment(char* start, char* end)
			{
				m_comment_start = start;
				m_comment_end = end;
			}
			int LeadingSpaces() { return m_leadingSpaces; }
			bool IsClosed()
			{
				if (m_valueNode)
				{
					return m_valueNode->IsClosed();
				}
				else
				{
					return m_end_pos != nullptr;
				}
			}
			void SetType(YamlNodeType t)
			{
				m_type = t;
			}
			void SetParent(YamlNode* pNode)
			{
				m_parent = pNode;
			}
			void SetValueNode(YamlNode* pNode)
			{
				m_valueNode = pNode;
				m_valueNode->SetParent(this);
			}
			YamlNode* Parent() { return m_parent; }
			YamlNode* ValueNode() { return m_valueNode; }
			void Add(YamlNode* pNode)
			{
				if (m_valueNode)
				{
					m_valueNode->Add(pNode);
				}
				else
				{
					m_children.push_back(pNode);
				}
			}
			YamlNodeType Type() { return m_type; }
			bool IsNullStartPos() { return m_start_pos == nullptr; }
		};
		class YamlParser
		{
			enum class eventType
			{
				Start,
				New_Sequence,
				End
			};
			char* m_data = nullptr;
			int m_size = 0;
			char* m_cur = nullptr;
			char* m_last = nullptr;
			bool m_bRun = false;

			YamlNode* m_root = nullptr;

			char getPrevChar()
			{
				if (m_cur > m_data)
				{
					return *(m_cur - 1);
				}
				else
				{
					return 0;
				}
			}
			char getChar()
			{
				char ch = 0;
				if (m_cur < m_last)
				{
					ch = *m_cur++;
				}
				else
				{
					throw eventType::End;
				}
				return ch;
			}
			char* getPos() { return m_cur; }
			YamlNode* Scan(YamlNode* pCurNode, Status& status);
			YamlNode* GetParentNode(YamlNode* pCurNode, Status& status);
		public:
			YamlParser();
			~YamlParser();
			bool Init();
			bool LoadFromString(char* code, int size);
			bool Parse();
			void Cleanup();
		};
	}
}
