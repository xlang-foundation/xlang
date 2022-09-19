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
	int token_startline = 0;
	int token_startCharPos = 0;
	int lineNo = 0;
	int charPos = 0;
	char prevChar = 0;
};
enum TokenIndex
{
	Token_False = 0,
	Token_True=1,
	Token_None=2,

	TokenInvalid = -1,
	TokenID = -10,
	TokenNum = -11,
	TokenStr = -12,
	TokenStrWithFormat = -102,
	TokenEOS = -13,
	TokenLineComment = -20,
	TokenComment = -21
};

enum class TokenErrorType
{
	WrongSlash,
};
struct TokenErrorInfo
{
	int lineNo;
	int charPos;
	TokenErrorType type;
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
	int leadingSpaceCnt;
	int index;
	int lineStart;
	int lineEnd;
	int charPos;
	int charStart;//offset from code begin
	int charEnd;//offset from code begin
};
class Token
{
	CoreContext _context;
	bool InSpace = false;
	bool InQuote = false;
	char quoteBeginChar = 0;
	bool InLineComment = false;
	bool InMatching = false;

	int begin_quoteCnt = 0;
	int end_quoteCnt = 0;

	inline char GetChar()
	{
		if (_context.spos > _context.src_code)
		{
			_context.prevChar = *(_context.spos - 1);
		}
		_context.charPos++;
		return *_context.spos++;
	}
	inline char PrevChar()
	{
		return _context.prevChar;
	}
	inline void IncLine()
	{
		_context.lineNo++;
		_context.charPos = 0;
	}
	inline void ResetToRoot()
	{
		_context.curNode = 0;
	}
	inline short GetLastMatchedNodeIndex()
	{
		node* pNode = (node*)(_context.kwTree + _context.curNode);
		short idx = pNode->index;
		if (idx == -1)
		{
			idx = TokenID;
		}
		return idx;
	}
	bool MatchInTree(char c);
	void ScanLineComment(char& c);
	void token_out(short idx,int offset =-1,bool callReset=true);
	inline void ifnotstart_token_start()
	{
		if (_context.token_start == nil)
		{
			new_token_start();
		}
	}
	inline void new_token_start(int addingOffset =0)
	{
		_context.token_start = _context.spos - 1+ addingOffset;
		_context.token_startline = _context.lineNo;
		_context.token_startCharPos = _context.charPos+ addingOffset-1;
	}
	inline void ClearToken()
	{
		_context.token_start = nil;
	}
	inline bool InStr(char c, const char* str)
	{
		if (c == 0) //for first char's pre-char
		{
			return true;
		}
		bool bYes = false;
		char* p = (char*)str;
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
	std::vector<TokenErrorInfo> m_errorInfos;
	void Scan();

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
	short Get(OneToken& one)
	{
		if (m_tokens.size() == 0)
		{
			Scan();
		}
		if (m_tokens.size() > 0)
		{
			one = m_tokens[0];
			short retIdx =  one.index;
			if (retIdx == TokenStr || retIdx == TokenStrWithFormat)
			{
				one.id.s += 1;//skip " or '
				one.id.size -= 2; //last " or '
			}
			m_tokens.erase(m_tokens.begin());
			return retIdx;
		}
		else
		{
			return TokenEOS;
		}
	}
	void Test();
};
}