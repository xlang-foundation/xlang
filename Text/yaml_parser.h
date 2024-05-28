#pragma once

#include <functional>
#include <unordered_map>
#include <vector>
#include <string>
#include "utility.h"

#if !defined(FORCE_INLINE)
#if defined(_MSC_VER)
// Microsoft Visual C++ Compiler
#define FORCE_INLINE __forceinline
#elif defined(__GNUC__) || defined(__clang__)
// GCC or Clang Compiler
#define FORCE_INLINE __attribute__((always_inline)) inline
#else
// Fallback for other compilers
#define FORCE_INLINE inline
#endif
#endif

namespace X
{
	namespace Text
	{
		enum class YamlNodeType: unsigned int
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
			bool inQuote = false;
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
			alignas(8) char* m_start_pos = nullptr;
			alignas(8) char* m_end_pos = nullptr;
			alignas(8) YamlNode* m_parent = nullptr;
			alignas(8) YamlNode* m_valueNode = nullptr; // Ensure pointer alignment
			std::vector<YamlNode*> m_children;
			alignas(4) YamlNodeType m_type = YamlNodeType::Node; // Typically requires less strict alignment
			alignas(4) int m_startLineNo = 0;
			alignas(4) int m_endLineNo = 0;
			alignas(4) int m_leadingSpaces = 0;
			alignas(4) int m_leadingTabs = 0;
			alignas(4) int m_inQuote = (int)false; // Treat boolean as int for alignment
			alignas(8) char* m_comment_start = nullptr;
			alignas(8) char* m_comment_end = nullptr;

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
			FORCE_INLINE bool HaveQuote()
			{
				return (bool)m_inQuote;
			}
			bool IsSingleValueType()
			{
				return (m_valueNode == nullptr) && (m_children.size() == 0);
			}
			void SetStartPos(char* p,int lineNo)
			{
				m_start_pos = p;
				m_startLineNo = lineNo;
			}
			void SetEndPos(char* p, int lineNo,bool inQuote)
			{
				if(m_valueNode)
				{ 
					m_valueNode->SetEndPos(p, lineNo, inQuote);
				}
				else
				{
					m_inQuote = (int)inQuote;
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
			FORCE_INLINE int GetChildrenCount()
			{
				return (int)m_children.size();
			}
			FORCE_INLINE std::vector<YamlNode*>& GetChildren()
			{
				return m_children;
			}
			FORCE_INLINE YamlNode* GetChild(int idx)
			{
				if (idx < 0 || idx >= m_children.size())
				{
					return nullptr;
				}
				return m_children[idx];
			}
			FORCE_INLINE YamlNode* GetValueNode()
			{
				return m_valueNode;
			}
			FORCE_INLINE std::string GetValue()
			{
				if (m_type == YamlNodeType::Doc)
				{
					return "";
				}
				else
				{
					 std::string strValue(m_start_pos, m_end_pos);
					 return trim(strValue);
				}
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
			YamlNode* FindNode(std::string keyName)
			{
				for (auto* pNode : m_children)
				{
					if (pNode->GetValue() == keyName)
					{
						return pNode;
					}
				}
				//check value node
				if (m_valueNode)
				{
					return m_valueNode->FindNode(keyName);
				}
				return nullptr;
			}
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
			YamlNode* GetRoot() { return m_root; }
		};
	}
}
