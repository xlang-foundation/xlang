#include "token.h"
#include <iostream>
#include <string>

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

void Token::ScanStringOrComments(char& c)
{
	char begin_c = c;
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
			if (begin_quoteCnt > 0 && c == begin_c)
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
	bool isComment = (begin_quoteCnt == 3)
		&& (end_quoteCnt == 3);
	token_out(isComment ? TokenComment : TokenStr);
}
void Token::token_out(short idx,int offset,bool callReset)
{
	if (_context.token_start == nil)
	{
		return;
	}
	//current char's previous char as end
	int size = int((_context.spos + offset) - _context.token_start);
	if (idx !=TokenEOS && size <= 0)
	{
		_context.token_start = nil;//_context.spos - 1;
		return;
	}
	OneToken one;
	one.id.s = _context.token_start;
	one.id.size = size;
	one.index = idx;
	one.leadingSpaceCnt = _context.leadingSpaceCount;
	one.lineStart = _context.token_startline;
	one.lineEnd = _context.lineNo;
	one.charPos = _context.token_startCharPos;
	_context.leadingSpaceCount = 0;
	m_tokens.push_back(one);
	if (callReset)
	{
		ResetToRoot();
	}
	_context.token_start = nil;//_context.spos - 1;
}
void Token::ScanLineComment(char& c)
{
	do
	{
		c = GetChar();
	} while (c != '\n' && c != 0);
	token_out(TokenLineComment);
}
void Token::ScanSpaces(char& c)
{
	int space = 0;
	do
	{
		space++;
		c = GetChar();
	} while (c == ' ' && c != 0);
	_context.leadingSpaceCount = space;
	new_token_start();
}
void Token::Scan()
{
	static const char* ops = "~`!@#$%^&*()-+={}[]|:;<>,.?/\t\r\n \\'\"#";
	static const char* split_chars = "\n \\'\"#";


	while (m_tokens.size() == 0)
	{
		char c = GetChar();
		if (c == 0)
		{
			token_out(GetLastMatchedNodeIndex());
			new_token_start();
			token_out(TokenEOS);
			break;
		}
		switch (c)
		{
		case ' ':
			if (!InQuote && !InLineComment)
			{
				if (!InSpace)
				{
					token_out(GetLastMatchedNodeIndex());
					_context.leadingSpaceCount = 0;
					InSpace = true;
					InMatching = false;
				}
				_context.leadingSpaceCount++;
			}
			break;
		case '\\':
			//just eat it
			break;
		case '\n':
			if (InQuote)
			{//meet other, break the 3-quotes rules like """ or '''
				if (begin_quoteCnt == 2)//empty string with ""  or ''
				{
					token_out(TokenStr);
					InQuote = false;
				}
				else if (begin_quoteCnt != 3)
				{
					begin_quoteCnt = 0;//reset
				}
			}
			else if (InLineComment)
			{
				token_out(TokenLineComment);
				InMatching = false;
				InLineComment = false;
			}
			else
			{
				if (PrevChar() != '\\')
				{//if not line continue case, out put line break also
				//output previous token if have
					token_out(GetLastMatchedNodeIndex());
					new_token_start();
					if (MatchInTree(c))
					{
						token_out(GetLastMatchedNodeIndex(),0);
					}
				}
				else
				{
					//output previous token if have,but skip the last slash
					token_out(GetLastMatchedNodeIndex(),-2);
					new_token_start(1);//skip \n
				}
				InMatching = false;
			}
			IncLine();
			break;
		case '\"':
		case '\'':
			if (!InQuote && InMatching)
			{
				token_out(GetLastMatchedNodeIndex());
				InMatching = false;
			}
			if (InQuote)
			{
				if (begin_quoteCnt == 3)
				{//count end quote to 3
					if (c == quoteBeginChar)
					{
						end_quoteCnt++;
						if (end_quoteCnt == 3)
						{
							token_out(TokenComment,0);
							InQuote = false;
						}
					}
					else
					{
						end_quoteCnt = 0;
					}
				}
				else
				{
					if (begin_quoteCnt >= 1 && c == quoteBeginChar)
					{//not break, continue to count
						begin_quoteCnt++;
					}
					else
					{//break, don't count again
						if (c == quoteBeginChar)
						{//meet end char
							token_out(TokenStr,0);
							InQuote = false;
						}
						else
						{
							begin_quoteCnt = 0;
						}
					}
				}
			}//end if (InQuote)
			else if (!InLineComment)
			{
				InQuote = true;
				quoteBeginChar = c;
				begin_quoteCnt = 1;
				end_quoteCnt = 0;
				new_token_start();
			}
			break;
		case '#':
			if (!InQuote)
			{
				if (InLineComment)
				{//meet again, invalid
				}
				else
				{
					token_out(GetLastMatchedNodeIndex());
					InLineComment = true;
					InMatching = false;
					new_token_start();
				}
			}
			break;
		default:
			if (InQuote)
			{//meet other, break the 3-quotes rules like """ or '''
				if (begin_quoteCnt == 2)//empty string with ""  or ''
				{
					token_out(TokenStr);
					InQuote = false;
				}
				else if(begin_quoteCnt!=3)
				{
					begin_quoteCnt = 0;//reset
				}
			}
			else if (!InLineComment)
			{
				ifnotstart_token_start();
				if (InMatching)
				{
					if (!MatchInTree(c))
					{
						if (InStr(PrevChar(), ops) || InStr(c, ops))
						{//except both are alpha chars, will start a new token
							token_out(GetLastMatchedNodeIndex());
							ifnotstart_token_start();
							InMatching = MatchInTree(c);//after reset
						}
						else
						{//for example class is a term, but input is classX
						//
							ResetToRoot();
							InMatching = false;
						}
					}
				}
				else
				{
					if (InStr(PrevChar(), ops) || InStr(c, ops))
					{//except both are alpha chars, will start a new token
						token_out(GetLastMatchedNodeIndex());
						ifnotstart_token_start();
						if (MatchInTree(c))
						{
							InMatching = true;
						}
					}
				}
			}
			break;
		}
	}
}
void Token::Scan2()
{
	//static const char ops[] = "~`!@#$%^&*()-+={}[]|\\:;\"'<>,.?/\t\r\n";
	static const char* ops = "~`!@#$%^&*()-+={}[]|:;<>,.?/\t\r";
	static const char* split_chars = "\n \\'\"#";
	bool bMatchStarted = true;
	if (_context.token_start == nil)
	{
		_context.token_start = _context.spos;
	}
	char c = 0;
	bool bEat = true;
	bool bFirstMatch = true;
	while (m_tokens.size() == 0)
	{
		if (bEat)
		{
			c = GetChar();
		}
		if (c == 0)
		{
			//push out last token
			token_out(GetLastMatchedNodeIndex());
			//EOS as a token to push out
			token_out(TokenEOS);
			break;
		}
		bool mKeepCheck = true;
		do
		{
			switch (c)
			{
			case '\n'://line break
				IncLine();
				mKeepCheck = false;
				break;
			case '\\'://line connection
				c = GetChar();
				if (c == '\n')
				{	//eat line break to connect next line
					IncLine();
					_context.token_start = _context.spos;
					c = GetChar();
					mKeepCheck = true;
				}
				else
				{
					m_errorInfos.push_back(TokenErrorInfo{
						_context.lineNo,_context.charPos-1,
						TokenErrorType::WrongSlash});
					//igore to find more errors
					mKeepCheck = true;
				}
				break;
			case ' ':
				token_out(GetLastMatchedNodeIndex());
				ScanSpaces(c);
				mKeepCheck = true;
				break;
			case '\'':
			case '\"':
				token_out(GetLastMatchedNodeIndex());
				ScanStringOrComments(c);//after this call, c must be ",' or 0
				//so don't need to loop back to check
				mKeepCheck = false;
				break;
			case '#':
				token_out(GetLastMatchedNodeIndex());
				ScanLineComment(c);
				break;
			default:
				mKeepCheck = false;
				break;
			}
		} while (c!=0 && mKeepCheck);
		if (c == 'c')
		{
			c = c;
		}
		if (!MatchInTree(c))
		{
			if (InStr(c, ops))
			{
				token_out(GetLastMatchedNodeIndex());
			}
			else if (InStr(PrevChar(), ops))
			{
				token_out(GetLastMatchedNodeIndex());
			}
			bFirstMatch = true;
		}
		else
		{
			if (bFirstMatch)
			{
				token_out(GetLastMatchedNodeIndex(),false);
			}
			bFirstMatch = false;
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

void Token::Test()
{
	int leadingSpaceCnt = 0;
	int idx = -1;
	while (idx != TokenEOS)
	{
		OneToken one;
		idx = Get(one);
		std::cout << "token:" << idx<< ",line:" 
			<< one.lineStart << ",pos:" << one.charPos;
		if (one.id.size > 0)
		{
			std::string str(one.id.s, one.id.size);
			std::cout <<"," << str;
			if (one.lineStart != one.lineEnd)
			{
				std::cout << ",end line:" << one.lineEnd;
			}
		}
		std::cout <<std::endl;
	}
}

}