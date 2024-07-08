#pragma once
#include "def.h"
#include <vector>

#if !defined(FORCE_INLINE)
#if defined(_MSC_VER)
// Microsoft Visual C++ Compiler
#define FORCE_INLINE __forceinline
#elif defined(BARE_METAL)
#define FORCE_INLINE 
#elif defined(__GNUC__) || defined(__clang__)
// GCC or Clang Compiler
#define FORCE_INLINE __attribute__((always_inline)) inline
#else
// Fallback for other compilers
#define FORCE_INLINE inline
#endif
#endif

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
	//count char( not space and not tab) per line start from first this kind char
	//reset when meet \n
	int lineCharCount = 0;
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
	TokenStrWithFormat = -121,
	TokenCharSequence = -122,
	TokenEOS = -13,
	TokenLineComment = -20,
	TokenComment = -21,
	TokenFeedOp = -22,
	TokenSpecialPosToBeLessOrEqual = -23,
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
	const char* OPS = "~`!@#$%^&*()-+={}[]|:;<>,.?/\t\r\n \\'\"#";
	CoreContext _context;
	
	//for some block like jit block, we need to pass through until meet a special pos
	//so use the two variables below to control it
	//if InMeetLineStartPosLessOrEqualToSpecialPos is true, 
	//then we will pass through until meet a line start pos <= SpecialPosToBeLessOrEqual
	bool InMeetLineStartPosLessOrEqualToSpecialPos = false;
	int SpecialPosToBeLessOrEqual = 0;

	bool InSpace = false;
	bool NotCharSequnce = false; //it is "...." not '....'
	bool InQuote = false;
	char quoteBeginChar = 0;
	bool InLineComment = false;
	bool InFeedOp = false;//for %
	bool InMatching = false;

	int begin_quoteCnt = 0;
	int end_quoteCnt = 0;
	//2 lines below for HTML parser
	bool SkipQuote = false;
	bool SkipHash = false; //#
	FORCE_INLINE char GetChar()
	{
		if (_context.spos > _context.src_code)
		{
			_context.prevChar = *(_context.spos - 1);
		}
		_context.charPos++;
		return *_context.spos++;
	}
	FORCE_INLINE char PrevChar()
	{
		return _context.prevChar;
	}
	FORCE_INLINE void IncLine()
	{
		_context.lineNo++;
		_context.charPos = 0;
		_context.lineCharCount = 0;
	}
	FORCE_INLINE void ResetToRoot()
	{
		_context.curNode = 0;
	}
	FORCE_INLINE short GetLastMatchedNodeIndex()
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
	void token_out(short idx,int offset =-1,bool callReset=true);
	FORCE_INLINE void ifnotstart_token_start()
	{
		if (_context.token_start == nil)
		{
			new_token_start();
		}
	}
	FORCE_INLINE void new_token_start(int addingOffset =0)
	{
		_context.token_start = _context.spos - 1+ addingOffset;
		_context.token_startline = _context.lineNo;
		_context.token_startCharPos = _context.charPos+ addingOffset-1;
	}
	FORCE_INLINE void ClearToken()
	{
		_context.token_start = nil;
	}
	FORCE_INLINE bool InStr(char c, const char* str)
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
	void SetSpecialPosToBeLessOrEqual(bool bEnable, int pos)
	{
		InMeetLineStartPosLessOrEqualToSpecialPos = true;
		SpecialPosToBeLessOrEqual = pos;
	}
	void set_ops(const char* ops)
	{
		OPS = ops;
	}
	void SetSkipQuote(bool b)
	{
		SkipQuote = b;
	}
	void SetSkipHash(bool b)
	{
		SkipHash = b;
	}
	void SetStream(char* code, int size)
	{
		_context.src_code = code;
		_context.src_code_size = size;
		_context.spos = _context.src_code;
	}
	//used by HTML Parser
	short UntilGet(std::string& key,OneToken& one)
	{
		const char* seq = key.c_str();
		int size = (int)key.size();
		int curMatechedIndex = 0;
		//matched part of exsited token( not take out)
		int offset = 0;
		if (_context.token_start != nil)
		{
			char* curCharPos = _context.token_start;
			while (curCharPos < _context.spos)
			{
				offset++;
				char c = *curCharPos++;
				if (c == seq[curMatechedIndex])
				{
					curMatechedIndex++;
					if (size == curMatechedIndex)
					{
						//todo: check this case
						token_out(TokenStr,0);
						break;
					}
				}
				else
				{//reset to beginning of key
					curMatechedIndex = 0;
				}
			}
		}
		new_token_start(-offset+1);//+1, because new_token_start already minus one
		while (true)
		{
			char c = GetChar();
			if (c == 0)
			{
				break;
			}
			if (c == seq[curMatechedIndex])
			{
				curMatechedIndex++;
				if (size == curMatechedIndex)
				{
					token_out(TokenStr,0);
					break;
				}
			}
			else
			{//reset to beginning of key
				curMatechedIndex = 0;
			}
		}
		if (m_tokens.size() > 0)
		{
			one = m_tokens[0];
			short retIdx = one.index;
			m_tokens.erase(m_tokens.begin());
			return retIdx;
		}
		else
		{
			return TokenEOS;
		}
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
			if (retIdx == TokenStr 
				|| retIdx == TokenStrWithFormat
				|| retIdx == TokenCharSequence)
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