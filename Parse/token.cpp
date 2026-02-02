/*
Copyright (C) 2024 The XLang Foundation
Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

	http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

#include "token.h"
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

		int lineBeginSpaceTabCount = 0;
		bool newTokenStartedForSpeicalPosMeet = false;

		// Helper lambda to check if char is a digit
		auto isDigit = [](char c) -> bool {
			return c >= '0' && c <= '9';
			};

		// Helper lambda to check if char is hex digit
		auto isHexDigit = [](char c) -> bool {
			return (c >= '0' && c <= '9') ||
				(c >= 'a' && c <= 'f') ||
				(c >= 'A' && c <= 'F');
			};

		// Helper lambda to check if we're currently building a numeric token
		// and if the given char should continue the number
		auto shouldContinueNumber = [&](char c, char prevC) -> bool {
			if (_context.token_start == nil) return false;

			// Get token so far
			int tokenLen = (int)(_context.spos - 1 - _context.token_start);
			if (tokenLen <= 0) return false;

			char firstChar = *_context.token_start;

			// Must start with digit to be a number
			if (!isDigit(firstChar)) return false;

			// Check if this is a hex number (0x...)
			bool isHexNumber = false;
			if (tokenLen >= 2 && firstChar == '0')
			{
				char secondChar = *(_context.token_start + 1);
				if (secondChar == 'x' || secondChar == 'X')
				{
					isHexNumber = true;
				}
			}

			// Case: decimal point in number
			if (c == '.' && isDigit(prevC))
			{
				// Peek at next char to see if it's a valid digit for this number type
				char nextC = *_context.spos;
				if (isHexNumber)
				{
					// For hex floats, next char can be hex digit or 'p'/'P'
					if (isHexDigit(nextC) || nextC == 'p' || nextC == 'P')
					{
						return true;
					}
				}
				else
				{
					// For decimal floats, next char must be digit
					if (isDigit(nextC))
					{
						return true;
					}
				}
				// If next char is not valid, don't treat . as part of number
				return false;
			}

			// Case: decimal point after hex digit (for hex float like 0x1.F)
			if (c == '.' && isHexNumber && isHexDigit(prevC))
			{
				char nextC = *_context.spos;
				if (isHexDigit(nextC) || nextC == 'p' || nextC == 'P')
				{
					return true;
				}
				return false;
			}

			// Case: digit after decimal point (e.g., "2" after "0.")
			if (isDigit(c) && prevC == '.')
			{
				// Check if there's a digit before the dot
				if (tokenLen >= 2)
				{
					char beforeDot = *(_context.spos - 3);
					if (isDigit(beforeDot)) return true;
				}
				else if (tokenLen == 1 && isDigit(firstChar))
				{
					return true;
				}
			}

			// Case: hex digit after decimal point in hex float (e.g., "F" after "0x1.")
			if (isHexNumber && isHexDigit(c) && prevC == '.')
			{
				return true;
			}

			// Case: 0x, 0X, 0b, 0B, 0o, 0O prefix
			if (tokenLen == 1 && firstChar == '0')
			{
				if (c == 'x' || c == 'X' || c == 'b' || c == 'B' || c == 'o' || c == 'O')
				{
					return true;
				}
			}

			// Case: hex digits after 0x (including p/P for hex float exponent)
			if (tokenLen >= 2 && firstChar == '0')
			{
				char secondChar = *(_context.token_start + 1);
				if (secondChar == 'x' || secondChar == 'X')
				{
					if (isHexDigit(c) || c == '_') return true;
					// p/P starts the exponent in hex float
					if (c == 'p' || c == 'P') return true;
				}
				else if (secondChar == 'b' || secondChar == 'B')
				{
					if (c == '0' || c == '1' || c == '_') return true;
				}
				else if (secondChar == 'o' || secondChar == 'O')
				{
					if ((c >= '0' && c <= '7') || c == '_') return true;
				}
			}

			// Case: +/- after p/P in hex float exponent
			if (isHexNumber && (c == '+' || c == '-') && (prevC == 'p' || prevC == 'P'))
			{
				return true;
			}

			// Case: digits after p, p+, p- in hex float
			if (isHexNumber && isDigit(c) && (prevC == 'p' || prevC == 'P' || prevC == '+' || prevC == '-'))
			{
				return true;
			}

			// Case: underscore in numbers (1_000_000)
			if (c == '_' && (isDigit(prevC) || (isHexNumber && isHexDigit(prevC)))) return true;
			if ((isDigit(c) || (isHexNumber && isHexDigit(c))) && prevC == '_') return true;

			// Case: scientific notation e/E (for decimal numbers only)
			if (!isHexNumber && (c == 'e' || c == 'E') && (isDigit(prevC) || prevC == '.'))
			{
				return true;
			}

			// Case: +/- after e/E in scientific notation
			if (!isHexNumber && (c == '+' || c == '-') && (prevC == 'e' || prevC == 'E'))
			{
				return true;
			}

			// Case: digits after e, e+, e-
			if (!isHexNumber && isDigit(c) && (prevC == 'e' || prevC == 'E' || prevC == '+' || prevC == '-'))
			{
				return true;
			}

			// Case: complex number suffix j/J
			if ((c == 'j' || c == 'J') && (isDigit(prevC) || prevC == '.' || (isHexNumber && isHexDigit(prevC))))
			{
				return true;
			}

			return false;
			};

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
					bool bCheckFString = !InMatching;
					if (InMatching)
					{
						// If matching, check if it's the start of token OR previous is OP
						long long termLen = (long long)(_context.spos - _context.token_start);
						if (termLen <= 1 || InStr(*_context.token_start, OPS))
						{
							bCheckFString = true;
						}
					}
					if (bCheckFString && (c == 'f' || c == 'F'))
					{
						char nextC = *_context.spos; //spos has been advanced by GetChar
						if (nextC == '"' || nextC == '\'')
						{
							if (InSpace)
							{
								InSpace = false;
								ClearToken();
							}
							if (InMatching)
							{
								token_out(GetLastMatchedNodeIndex());
								InMatching = false;
								ResetToRoot();
							}
							_context.leadingSpaceCount = 0;
							//Force set token start to current 'f'
							//because ifnotstart_token_start might be confused by above token_out
							_context.token_start = _context.spos - 1;
							
							InFString = true;
							NotCharSequnce = true;
							InQuote = true;
							quoteBeginChar = nextC;
							begin_quoteCnt = 1;
							end_quoteCnt = 0;
							//current token start is already set by ifnotstart_token_start()
							//and it points to 'f'
							GetChar();//consume quote
							return;
						}
					}
					if (InMatching)
					{
						if (!MatchInTree(c))
						{
							if (InStr(PrevChar(), OPS) || InStr(c, OPS))
							{
								// Check if this should continue a numeric literal
								if (!shouldContinueNumber(c, PrevChar()))
								{
									//except both are alpha chars, will start a new token
									token_out(GetLastMatchedNodeIndex());
									ifnotstart_token_start();
									InMatching = MatchInTree(c);//after reset
								}
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
						{
							// Check if this should continue a numeric literal
							if (!shouldContinueNumber(c, p_c))
							{
								//except both are alpha chars, will start a new token
								token_out(GetLastMatchedNodeIndex());
								ifnotstart_token_start();
								if (MatchInTree(c))
								{
									InMatching = true;
								}
							}
						}
					}
				}
			};

		bool bom_checked = false;

		while (m_tokens.size() == 0)
		{
			char c = GetChar();

			if (!bom_checked)
			{
				bom_checked = true;
				if (c == static_cast<char>(0xEF))
				{
					char c2 = GetChar();
					char c3 = GetChar();
					if (c2 == static_cast<char>(0xBB) && c3 == static_cast<char>(0xBF)) {
						// BOM detected, skip these characters and move to the next character
						c = GetChar();
					}
				}
			}

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
			//CASE:for special pos to meet
			if (InMeetLineStartPosLessOrEqualToSpecialPos)
			{
				bool Loopback = true;
				if (!newTokenStartedForSpeicalPosMeet)
				{
					newTokenStartedForSpeicalPosMeet = true;
					new_token_start();
				}
				if (c == ' ' || c == '\t')
				{
					lineBeginSpaceTabCount++;
				}
				else if (c == '\n')
				{
					IncLine();
					lineBeginSpaceTabCount = 0;
				}
				else
				{
					if (lineBeginSpaceTabCount <= SpecialPosToBeLessOrEqual)
					{
						//if not space or tab, and line char count less or equal to special pos
						//it means we need the pos we wanted
						token_out(TokenSpecialPosToBeLessOrEqual);
						InMeetLineStartPosLessOrEqualToSpecialPos = false;
						SpecialPosToBeLessOrEqual = 0;
						//we need to count this char back, so set to -1
						new_token_start();
						//if (MatchInTree(c))
						//{
						//	token_out(GetLastMatchedNodeIndex(), 0);
						//}
						//break;
						Loopback = false;
					}
				}
				//continue to next char to check
				if (Loopback)
				{
					continue;
				}
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
				else if (c != '\n' && c != ' ' && c != '\t' && c != '\r')
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
			//also reset end_quoteCnt if c is not quoteBeginChar
			if (end_quoteCnt > 0 && c != quoteBeginChar)
			{
				end_quoteCnt = 0;
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
						token_out((meetDollar || meetSlash || haveEscapeCode)
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
					//and push this car(\n) to tree match
					//will gerenate a operator (new line op)
					new_token_start();
					if (MatchInTree(c))
					{
						token_out(GetLastMatchedNodeIndex(), 0);
					}
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
						//and push this car(\n) to tree match
						//will gerenate a operator (new line op)
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
									? (InFString ? TokenStrFmt : TokenStrWithFormat) :
									(NotCharSequnce ? (InFString ? TokenStrFmt : TokenStr) : TokenCharSequence), 0);
								NotCharSequnce = false;
								InQuote = false;
								InFString = false;
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