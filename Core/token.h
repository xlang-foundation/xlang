#pragma once
#include "def.h"
#include <vector>

namespace X {
enum LastCharType
{
	LCT_None,
	LCT_Space,
	LCT_Str,//with ''
	LCT_Str2,//with " "
	LCT_Ops,
	LCT_Alpha,//digit and alphabet
	LCT_LineComment,//With #
	LCT_EOS//End of Stream
};
struct CoreContext
{
	short* kwTree=nil;

	char* src_code=nil;
	int src_code_size=0;

	//Cursor
	char* spos=nil;//source pos
	short curNode=0;//offset in kwTree
	int leadingSpaceCount = 0;
	LastCharType lct = LastCharType::LCT_None;
	char* token_start=0;
};
enum TokenIndex
{
	TokenInvalid = -1,
	TokenID = -10,
	TokenNum = -11,
	TokenStr = -12,
	TokenEOS = -13,
	TokenLineComment = -20,
	TokenComment = -21
};

#pragma pack(push,1)
struct node
{
	short index;
	unsigned char c;
	unsigned char child_cnt;
};
#pragma pack(pop)

struct OneToken
{
	String id;
	short leadingSpaceCnt;
	short index;
};
class Token
{
	CoreContext _context;
	inline char GetChar()
	{
		return *_context.spos++;
	}
	inline void ResetToRoot()
	{
		_context.curNode = 0;
	}
	inline short GetLastMatchedNodeIndex()
	{
		node* pNode = (node*)(_context.kwTree + _context.curNode);
		return pNode->index;
	}
	bool MatchInTree(char c);
	inline bool InStr(char c, char* str)
	{
		bool bYes = false;
		char* p = str;
		while (*p)
		{
			if (*p == c)
			{
				bYes = true;
				break;
			}
			p++;
		}
		return bYes;
	}
	std::vector<OneToken> m_tokens;
	void Scan();
	short Scan1(String& id, int& leadingSpaceCnt);

public:
	Token(short* kwTree)
	{
		_context.kwTree = kwTree;
		_context.lct = LCT_None;
		_context.curNode = 0;
		_context.token_start = nil;
	}
	void SetStream(char* code, int size)
	{
		_context.src_code = code;
		_context.src_code_size = size;
		_context.spos = _context.src_code;
	}
	short Get(String& tk, int& leadingSpaceCnt)
	{
		if (m_tokens.size() == 0)
		{
			Scan();
		}
		if (m_tokens.size() > 0)
		{
			OneToken& one = m_tokens[0];
			tk = one.id;
			leadingSpaceCnt = one.leadingSpaceCnt;
			short retIdx =  one.index;
			m_tokens.erase(m_tokens.begin());
			return retIdx;
		}
		else
		{
			return TokenEOS;
		}
	}
};
}