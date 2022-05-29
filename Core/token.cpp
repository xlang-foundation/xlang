#include "token.h"
namespace XPython {

bool Token::MatchInTree(char c)
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
short Token::Scan(String& id)
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
		id.size = int(_context.spos - id.s - 1);

		//reset for next
		ResetToRoot();
		_context.token_start = _context.spos;
		_context.lct = LCT_None;

		return TokenStr;
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
			id.size = int(_context.spos - _context.token_start - 1);
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
					id.size = int(_context.spos - _context.token_start - 1);
				}
				else
				{
					id.s = nil;
					id.size = 0;
				}
				_context.token_start = _context.spos - 1;
			}
			ResetToRoot();
			if (lct == LCT_Ops)
			{
				b = MatchInTree(c);
			}
		}
	}
	return retIndex;
}

}