#include "pycore.h"

#include <iostream>
#include <string>

#define nil 0

enum LastCharType
{
	LCT_None,
	LCT_Space,
	LCT_Str,//with ''
	LCT_Str2,//with " "
	LCT_Ops,
	LCT_Alpha,//digit and alphabet
	LCT_EOS//End of Stream
};
struct PyCoreContext
{
	short* kwTree;

	char* src_code;
	int src_code_size;

	//Cursor
	char* spos;//source pos
	short curNode;//offset in kwTree
	LastCharType lct;
	char* token_start;
};

static PyCoreContext _context;
struct Node
{
	char tag;//char for this node
	unsigned char child_cnt;//max is 256, because only one byte per node
	unsigned short child_ptrs[];//number is same as child_cnt
};

struct String
{
	char* s;
	int size;
};

enum TokenIndex
{
	TokenID = -10,
	TokenNum = -11,
	TokenStr = -12,
	TokenEOS = -13
};
#pragma pack(push,1)
struct node
{
	short index;
	unsigned char c;
	unsigned char child_cnt;
};
#pragma pack(pop)

inline char GetChar()
{
	return *_context.spos++;
}
inline void ResetToRoot()
{
	_context.curNode = 0;
}
short GetLastMatchedNodeIndex()
{
	node* pNode = (node*)(_context.kwTree + _context.curNode);
	return pNode->index;
}
bool MatchInTree(char c)
{
	bool bMatched = false;
	node* pNode = (node*)(_context.kwTree + _context.curNode);
	short* pChildStart = _context.kwTree + _context.curNode + 2;
	//check if match with one child
	for (int i = 0; i < pNode->child_cnt; i++)
	{
		short offset = *(pChildStart + i);
		node* pChildNode = (node*)(_context.kwTree + offset);
		if (pChildNode->c == c)
		{
			_context.curNode = offset;
			bMatched = true;
			break;
		}
	}
	return bMatched;
}
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
short GetToken(String& id)
{
	static const char ops[] = "~`!@#$%^&*()-+={}[]|\\:;\"'<>,.?/\t\r\n";
	id.s = nil;
	id.size = 0;
	char c;
	short retIndex = -1;
	if (LCT_EOS == _context.lct)
	{
		return TokenEOS;
	}
	else if (LCT_Str == _context.lct || LCT_Str2 == _context.lct)
	{//fetch all string and return token
		char begin_c = (LCT_Str == _context.lct) ? '\'' : '\"';
		id.s = _context.spos;
		do
		{
			c = GetChar();
		} while (c != begin_c && c != 0);
		id.size = _context.spos - id.s - 1;

		retIndex = _context.lct;
		//reset for next
		ResetToRoot();
		_context.token_start = _context.spos;
		_context.lct = LCT_None;

		return retIndex;
	}
	c = GetChar();

	LastCharType lct = LCT_None;
	if (c == 0)
	{//end
		lct = LCT_EOS;
	}
	else if (c == '\'')
	{
		lct = LCT_Str;
	}
	else if (c == '\"')
	{
		lct = LCT_Str2;
	}
	else if (c == ' ')
	{
		lct = LCT_Space;
	}
	else if (InStr(c, (char*)ops))
	{
		lct = LCT_Ops;
	}
	else
	{
		lct = LCT_Alpha;
	}
	if (_context.lct != lct)
	{//new token begins,fetch last token
		short idx = GetLastMatchedNodeIndex();
		if (idx != -1)
		{
			retIndex = idx;
		}
		else
		{
			retIndex = TokenID;
		}
		if (_context.token_start != nil)
		{
			id.s = _context.token_start;
			id.size = _context.spos - _context.token_start - 1;
		}
		else
		{
			id.s = nil;
			id.size = 0;
		}
		//prepare for next
		ResetToRoot();
		_context.token_start = _context.spos - 1;
	}
	if (c == ' ')
	{
		do
		{
			c = GetChar();
		} while (c == ' ');
		if (c == 0)
		{//end
			lct = LCT_EOS;
		}
		else if (InStr(c, (char*)ops))
		{
			lct = LCT_Ops;
		}
		else
		{
			lct = LCT_Alpha;
		}
		//new token starts
		_context.token_start = _context.spos - 1;
	}
	_context.lct = lct;

	if (c != ' ' && lct != LCT_Str && lct != LCT_Str2)
	{
		bool b = MatchInTree(c);
		if (!b)
		{
			if (lct == LCT_Ops)
			{//in this case, the previus char should be Ops also
			//so output a token 
				short idx = GetLastMatchedNodeIndex();
				if (idx != -1)
				{
					retIndex = idx;
				}
				else
				{
					retIndex = TokenID;
				}
				if (_context.token_start != nil)
				{
					id.s = _context.token_start;
					id.size = _context.spos - _context.token_start - 1;
				}
				else
				{
					id.s = nil;
					id.size = 0;
				}
				_context.token_start = _context.spos - 1;
			}
			ResetToRoot();
		}
	}
	return retIndex;
}


void PyInit(short* kwTree)
{
	_context.kwTree = kwTree;
	_context.lct = LCT_None;
	_context.curNode = 0;
	_context.token_start = nil;
}


PyHandle PyLoad(char* code, int size)
{
	_context.src_code = code;
	_context.src_code_size = size;
	_context.spos = _context.src_code;
	std::cout << "Token->" << std::endl;
	while (true)
	{
		String s;
		short idx = GetToken(s);
		if (idx == TokenEOS)
		{
			break;
		}
		if (s.s !=nil && s.size > 0)
		{
			std::string str(s.s, s.s + s.size);
			std::cout <<"Id:"<<idx<<",Val:" << str << std::endl;
		}
		idx = idx;
	}

	return PyHandle();
}

bool PyRun(PyHandle h)
{
	return false;
}

void PyClose(PyHandle)
{
}
