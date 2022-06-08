#include "token.h"
#include <vector>
namespace X {

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
void Token::Scan()
{
	static const char ops[] = "~`!@#$%^&*()-+={}[]|\\:;\"'<>,.?/\t\r\n";
	bool bMatchStarted = true;

	static auto token_out = [&]()
	{
		OneToken one;
		one.id.s = _context.token_start;
		one.id.size = int(_context.spos - _context.token_start - 1);
		one.index = GetLastMatchedNodeIndex();
		m_tokens.push_back(one);
		ResetToRoot();
		_context.token_start = _context.spos-1;
	};
	static auto new_token_start = [&]()
	{
		_context.token_start = _context.spos - 1;
	};

	if (_context.token_start == nil)
	{
		_context.token_start = _context.spos;
	}
	while (true)
	{
		char c = GetChar();
		if (c == 0)
		{
			break;
		}
		switch (c)
		{
		case ' ':
			token_out();
			do
			{
				c = GetChar();
			} while (c == ' ' && c !=0);
			new_token_start();
			break;
		case '\"':
			token_out();
			break;
		case '\'':
			token_out();
			break;
		case '#':
			token_out();
			break;
		default:
			break;
		}
		if (InStr(c, (char*)ops))
		{
			if (!bMatchStarted)
			{//output 
				token_out();
				bMatchStarted = true;
			}
			bool b = MatchInTree(c);
			if (!b)
			{//maybe c is the new token's start char
				token_out();
				bMatchStarted = true;
				b = MatchInTree(c);
			}
		}
		else if(bMatchStarted)
		{
			bool b = MatchInTree(c);
			if (!b)
			{
				ResetToRoot();
				bMatchStarted = false;
			}
		}
		if (m_tokens.size() > 0 || c == 0)
		{
			break;
		}
	}
}
short Token::Scan1(String& id, int& leadingSpaceCnt)
{
	static const char ops[] = "~`!@#$%^&*()-+={}[]|\\:;\"'<>,.?/\t\r\n";
	id.s = nil;
	id.size = 0;
	char c;
	short retIndex = -1;
	if (LCT_EOS == _context.lct)
	{
		leadingSpaceCnt = _context.leadingSpaceCount;
		return TokenEOS;
	}
	else if (LCT_Str == _context.lct 
		|| LCT_Str2 == _context.lct)
	{//fetch all string and return token
		char begin_c = (LCT_Str == _context.lct) 
			? '\'' : '\"';
		id.s = _context.spos;
		//do ''' ''' or """ """as comment
		int begin_quoteCnt = 1;
		int end_quoteCnt = 0;
		do
		{
			c = GetChar();
			if (begin_quoteCnt == 3)
			{//count end quote to 3
				if (c == begin_c)
				{
					end_quoteCnt++;
					if (end_quoteCnt == 3)
					{
						break;
					}
					else
					{
						c = 1;//not break the while
						continue;
					}
				}
				else
				{
					end_quoteCnt = 0;
				}
			}
			else
			{
				if (begin_quoteCnt >0 && c == begin_c)
				{
					begin_quoteCnt++;
					c = 1;//not break the while
					continue;
				}
				else
				{//break, don't count again
					begin_quoteCnt = 0;
				}
			}
		} while (c != begin_c && c != 0);
		id.size = int(_context.spos - id.s - 1);
		leadingSpaceCnt = _context.leadingSpaceCount;
		bool isComment = (begin_quoteCnt ==3) 
			&& (end_quoteCnt==3);

		//reset for next
		ResetToRoot();
		_context.token_start = _context.spos;
		_context.leadingSpaceCount = 0;
		_context.lct = LCT_None;

		return isComment? TokenComment:TokenStr;
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
		leadingSpaceCnt = _context.leadingSpaceCount;
		//prepare for next
		ResetToRoot();
		_context.token_start = _context.spos - 1;
		_context.leadingSpaceCount = 0;
	}
	if (c == ' ')
	{
		int sp_cnt = 0;
		do
		{
			sp_cnt++;
			c = GetChar();
		} while (c == ' ');
		_context.leadingSpaceCount = sp_cnt;
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
	else if (c == '#')
	{
		id.s = _context.spos;
		do
		{
			c = GetChar();
		} while (c != '\n' && c != 0);
		ResetToRoot();
		if (c != 0)
		{//put \n into tree
			MatchInTree(c);
		}
		id.size = int(_context.spos - id.s - 1);
		//TODO: maybe LCT_LineComment
		_context.lct = (c==0)? LCT_EOS:LCT_Ops;
		retIndex = TokenLineComment;
		//new token starts
		_context.token_start = _context.spos - 1;
		_context.leadingSpaceCount = 0;

		return retIndex;
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
				leadingSpaceCnt = _context.leadingSpaceCount;
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