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

	void Token::token_out(short idx, int offset, bool callReset)
	{
		if (_context.token_start == nil)
		{
			return;
		}
		//current char's previous char as end
		int size = int((_context.spos + offset) - _context.token_start);
		if (idx != TokenEOS && size <= 0)
		{
			ClearToken();
			return;
		}
		OneToken one;
		one.charStart = int(_context.token_start - _context.src_code);
		one.charEnd = one.charStart + size;
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
		ClearToken();
	}

	void Token::Scan()
	{
		//also flag if inside Quote, find \x or ${ for var inside string case
		bool meetDollar = false;
		bool meetSlash = false;
		bool haveEscapeCode = false;//like \n,\r...

		auto default_proc = [&](char c)
		{
			if (InSpace)
			{
				InSpace = false;
				ClearToken();
			}
			if (InQuote)
			{//meet other, break the 3-quotes rules like """ or '''
				if (begin_quoteCnt == 2)//empty string with ""  or ''
				{
					token_out((meetDollar || meetSlash || haveEscapeCode)
						? TokenStrWithFormat :
						(NotCharSequnce ? TokenStr : TokenCharSequence));
					NotCharSequnce = false;
					InQuote = false;
					//also reset lines below for string
					meetDollar = false;
					meetSlash = false;
					haveEscapeCode = false;
					//begin_quoteCnt = 0;//reset
					//this string finished, continue run to check the current char
				}
				else if (begin_quoteCnt != 3)
				{
					//not begin with 3 " which means such pattern""" .... """
					//so reset to 0 as regular string
					begin_quoteCnt = 0;//reset
					//go back to next char, because it is still inside string
					return;
				}
				else
				{
					//go back to next char, because it is still inside string
					return;
				}
			}
			if (c == '%' && _context.lineCharCount == 1)
			{
				token_out(GetLastMatchedNodeIndex());
				InFeedOp = true;
				InMatching = false;
				new_token_start();
			}
			else if (!InLineComment && !InFeedOp)
			{
				ifnotstart_token_start();
				if (InMatching)
				{
					if (!MatchInTree(c))
					{
						if (InStr(PrevChar(), OPS) || InStr(c, OPS))
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
					char p_c = PrevChar();
					if (InStr(p_c, OPS) || InStr(c, OPS))
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
		};

		while (m_tokens.size() == 0)
		{
			char c = GetChar();
			if (c == 0)
			{
				if (InFeedOp)
				{
					token_out(TokenFeedOp);
					InFeedOp = false;
				}
				else
				{
					token_out(GetLastMatchedNodeIndex());
				}
				new_token_start();
				token_out(TokenEOS);
				break;
			}
			//to cover case after slash, there are some spaces or tabs
			//if not, turn off this flag
			//and in newline meets, just check this flag
			if (meetSlash)
			{
				if ((c == 'n' || c == 't' || c == 'r') && PrevChar() == '\\')
				{
					if (InQuote)
					{
						haveEscapeCode = true;
					}
					meetSlash = false;
				}
				else if (c != '\n' && c != ' ' && c != '\t')
				{
					meetSlash = false;
				}
			}
			if (c != ' ' && c != '\t')
			{//todo: if inside quote, how to do?
				_context.lineCharCount++;
			}
			//process case ${vars} inside string
			if (c == '{')
			{
				if (InQuote && PrevChar() == '$')
				{
					meetDollar = true;
					continue;
				}
			}
			switch (c)
			{
			case '\\':
				if (InSpace)
				{
					InSpace = false;
					ClearToken();
				}
				if (InQuote || InFeedOp)
				{
					meetSlash = true;
				}
				//just eat it
				break;
			case '\n':
				if (InSpace)
				{
					InSpace = false;
					ClearToken();
				}
				if (InQuote)
				{//meet other, break the 3-quotes rules like """ or '''
					if (begin_quoteCnt == 2)//empty string with ""  or ''
					{
						token_out((meetDollar || meetSlash|| haveEscapeCode)
							? TokenStrWithFormat :
							(NotCharSequnce ? TokenStr : TokenCharSequence), -1);
						NotCharSequnce = false;
						InQuote = false;
						new_token_start();
						if (MatchInTree(c))
						{
							token_out(GetLastMatchedNodeIndex(), 0);
						}
						//also reset lines below for string
						meetDollar = false;
						meetSlash = false;
						haveEscapeCode = false;
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
				else if (InFeedOp && !meetSlash)
				{
					token_out(TokenFeedOp);
					InMatching = false;
					InFeedOp = false;
				}
				else
				{
					//old:if (PrevChar() != '\\')
					if (!meetSlash)
					{//if not line continue case, output line break also
					//output previous token if have
						if (InFeedOp)
						{
							token_out(TokenFeedOp);
							InFeedOp = false;
						}
						else
						{
							token_out(GetLastMatchedNodeIndex());
						}
						new_token_start();
						if (MatchInTree(c))
						{
							token_out(GetLastMatchedNodeIndex(), 0);
						}
					}
					else if (!InFeedOp)
					{
						//output previous token if have,but skip the last slash
						token_out(GetLastMatchedNodeIndex(), -2);
						new_token_start(1);//skip \n
					}
					InMatching = false;
				}
				meetSlash = false;//reset
				IncLine();
				break;
			case '\"':
			case '\'':
				if (SkipQuote)
				{
					continue;
				}
				NotCharSequnce = (c == '\"');
				if (InSpace)
				{
					InSpace = false;
					ClearToken();
				}
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
								token_out(TokenComment, 0);
								InQuote = false;
								//also reset lines below for string
								meetDollar = false;
								meetSlash = false;
								haveEscapeCode = false;
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
								token_out((meetDollar || meetSlash || haveEscapeCode)
									? TokenStrWithFormat :
									(NotCharSequnce ? TokenStr : TokenCharSequence), 0);
								NotCharSequnce = false;
								InQuote = false;
								//also reset lines below for string
								meetDollar = false;
								meetSlash = false;
								haveEscapeCode = false;
							}
							else
							{
								begin_quoteCnt = 0;
							}
						}
					}
				}//end if (InQuote)
				else if (!InLineComment && !InFeedOp)
				{
					InQuote = true;
					quoteBeginChar = c;
					begin_quoteCnt = 1;
					end_quoteCnt = 0;
					new_token_start();
				}
				break;
			case '#':
				//if Skip Hash, continue to next case ,will go to default
				if (SkipHash)
				{
					default_proc(c);
				}
				else
				{
					if (InSpace)
					{
						InSpace = false;
						ClearToken();
					}
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
				}
				break;
			case ' ':
				if (!InQuote && !InLineComment && !InFeedOp)
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
				else
				{
					default_proc(c);
				}
				break;
			default:
				default_proc(c);
				break;
			}
		}
	}

	void Token::Test()
	{
		int leadingSpaceCnt = 0;
		int idx = -1;
		while (idx != TokenEOS)
		{
			OneToken one;
			idx = Get(one);
			std::cout << "token:" << idx << ",line:"
				<< one.lineStart << ",pos:" << one.charPos;
			if (one.id.size > 0)
			{
				std::string str(one.id.s, one.id.size);
				std::cout << "," << str;
				if (one.lineStart != one.lineEnd)
				{
					std::cout << ",end line:" << one.lineEnd;
				}
			}
			std::cout << std::endl;
		}
	}

}