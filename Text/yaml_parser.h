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
		public:
			YamlNode(char* startPos, int lineNo)
			{
				m_start_pos = startPos;
				m_startLineNo = lineNo;
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

		struct Status
		{
			bool NewLine = false;
			int lineNo = 1;
			int LeadingSpaces = 0;
			int LeadingTabs = 0;
		};
		class YamlParser
		{
			typedef std::function<void(int)> EventHandler;
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
			std::unordered_map<eventType, EventHandler> m_Handlers;
			std::vector<eventType> m_events;
			void Fire(eventType evt)
			{
				m_events.push_back(evt);
			}
			void On(eventType evt, EventHandler func)
			{
				m_Handlers.emplace(std::make_pair(evt,func));
			}
			void Process();
			bool m_bRun = false;

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
			YamlNode* GetParentNode3(YamlNode* pCurNode, Status& status);
			YamlNode* GetParentNode2(YamlNode* pCurNode, Status& status);
			bool IsValueNode(YamlNode* pNode);
		public:
			YamlParser();
			~YamlParser();
			bool Init();
			bool LoadFromString(char* code, int size);
			bool Parse();
			bool Parse3();
			bool Parse2();
		};
	}
}
