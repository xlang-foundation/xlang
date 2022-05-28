#include "pycore.h"
#include "exp.h"
#include <iostream>
#include <string>
#include <stack>
#include <vector>

namespace XPython {

enum class ParseState
{
	Wrong_Fmt,
	Null,
	Non_Number,
	Double,
	Long_Long
};

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

struct Number
{

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

static int _precedence[(short)KWIndex::MaxCount];
inline int Precedence(short idx)
{
	return _precedence[idx];
}
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


short Scan(String& id)
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
			if (lct == LCT_Ops)
			{
				b = MatchInTree(c);
			}
		}
	}
	return retIndex;
}
short GetToken(String& id)
{
	short retIndex = -1;
	while (retIndex == -1)
	{
		retIndex = Scan(id);
	}
	return retIndex;
}

void PyInit(short* kwTree)
{
	_context.kwTree = kwTree;
	_context.lct = LCT_None;
	_context.curNode = 0;
	_context.token_start = nil;

	for (short i = 0; i < (short)KWIndex::MaxCount; i++)
	{
		_precedence[i] = 1000;
	}
	for (short i = (short)KWIndex::Minus+1;
		i <= (short)KWIndex::Arithmetic_Op_S; i++)
	{//for "*","/","%","**","//"
		_precedence[i] = 1001;
	}
	_precedence[(short)KWIndex::Colon] = 1002;
	for (short i = (short)KWIndex::Assign_Op_S;
		i <= (short)KWIndex::Assign_Op_E; i++)
	{
		_precedence[i] = 0;
	}
}

enum num_state
{
	num_state_first_zero,

};
ParseState ParseHexBinOctNumber(String& str)
{
	return ParseState::Null;
}

ParseState ParseNumber(String& str,double& dVal,long long& llVal)
{
	ParseState st = ParseState::Null;
	if (str.s == nil || str.size == 0)
	{
		return st;
	}
	if (str.size >= 2 && *str.s =='0' )
	{
		char c = *(str.s + 1);
		//for hex(0x),oct(0o),bin(0b), also require first letter is 0
		//so this is true if it is number
		if (c == 'x' || c == 'o' || c == 'b')
		{
			String str2 = { str.s + 2,str.size - 2 };
			return ParseHexBinOctNumber(str2);
		}
	}
	long long primary[2]={0,0};
	int digit_cnt[2] = { 0,0 };
	char* end = str.s + str.size;
	int it = 0;
	bool meetDot = false;
	bool correctSyntax = true;
	char* p = str.s;
	while(p < end)
	{
		char c = *p++;
		if (c >= '0' && c <= '9')
		{
			primary[it] = primary[it] * 10 + c - '0';
			digit_cnt[it]++;
		}
		else if (c == '.')
		{
			if (meetDot)
			{//more than one
				//error
				correctSyntax = false;
				break;
			}
			meetDot = true;
			it++;
		}
		else
		{
			//error
			correctSyntax = false;
			break;
		}
	}
	if (correctSyntax)
	{
		if (meetDot)
		{
			dVal = primary[1];
			for (int i = 0; i < digit_cnt[1];i++)
			{
				dVal /= 10;
			}
			dVal += primary[0];
			st = ParseState::Double;
		}
		else
		{
			st = ParseState::Long_Long;
			llVal = primary[0];
			st = ParseState::Long_Long;
		}
	}
	return st;
}

void DoOpTop(std::stack<AST::Expression*>& operands,
	std::stack<AST::Operator*>& ops)
{
	auto top = ops.top();
	ops.pop();
	if (top->getOp() == (short)KWIndex::Colon)
	{//:
		auto operandR = operands.top();
		operands.pop();
		auto operandL = operands.top();
		operands.pop();
		auto param = new AST::Param(operandL, operandR);
		operands.push(param);
	}
	else if (top->getOp() == (short)KWIndex::Comma)
	{//,
		auto operandR = operands.top();
		operands.pop();
		auto operandL = operands.top();
		operands.pop();
		AST::List* list = nil;
		if (operandL->m_type != AST::ObType::List)
		{
			list = new AST::List(operandL);
		}
		else
		{
			list = (AST::List*)operandL;
		}
		if (operandR->m_type != AST::ObType::List)
		{
			*list += operandR;
		}
		else
		{
			*list += (AST::List*)operandR;
			delete operandR;
		}
		operands.push(list);
	}
	else if (top->m_type == AST::ObType::Func)
	{
		AST::Func* func = (AST::Func*)top;
		auto params = operands.top();
		operands.pop();
		if (params->m_type != AST::ObType::List)
		{
			AST::List* list = new AST::List(params);
			delete params;
			func->SetParams(list);
		}
		else
		{
			func->SetParams((AST::List*)params);
		}
		func->SetName(operands.top());
		operands.pop();
		operands.push(func);//as new block's head object
	}
	else if (top->m_type == AST::ObType::UnaryOp)
	{
		auto operand = operands.top();
		operands.pop();
		((AST::UnaryOp*)top)->SetR(operand);
		operands.push(top);
	}
	else if (top->m_type == AST::ObType::Assign)
	{
		auto operandR = operands.top();
		operands.pop();
		auto operandL = operands.top();
		operands.pop();
		((AST::Assign*)top)->SetLR(operandL, operandR);
		operands.push(top);
	}
	else if (top->m_type == AST::ObType::BinaryOp)
	{
		auto operandR = operands.top();
		operands.pop();
		auto operandL = operands.top();
		operands.pop();
		((AST::BinaryOp*)top)->SetLR(operandL, operandR);
		operands.push(top);
	}
}

/* from https://www.geeksforgeeks.org/expression-evaluation/
1. While there are still tokens to be read in,
   1.1 Get the next token.
   1.2 If the token is:
	   1.2.1 A number: push it onto the value stack.
	   1.2.2 A variable: get its value, and push onto the value stack.
	   1.2.3 A left parenthesis: push it onto the operator stack.
	   1.2.4 A right parenthesis:
		 1 While the thing on top of the operator stack is not a
		   left parenthesis,
			 1 Pop the operator from the operator stack.
			 2 Pop the value stack twice, getting two operands.
			 3 Apply the operator to the operands, in the correct order.
			 4 Push the result onto the value stack.
		 2 Pop the left parenthesis from the operator stack, and discard it.
	   1.2.5 An operator (call it thisOp):
		 1 While the operator stack is not empty, and the top thing on the
		   operator stack has the same or greater precedence as thisOp,
		   1 Pop the operator from the operator stack.
		   2 Pop the value stack twice, getting two operands.
		   3 Apply the operator to the operands, in the correct order.
		   4 Push the result onto the value stack.
		 2 Push thisOp onto the operator stack.
2. While the operator stack is not empty,
	1 Pop the operator from the operator stack.
	2 Pop the value stack twice, getting two operands.
	3 Apply the operator to the operands, in the correct order.
	4 Push the result onto the value stack.
3. At this point the operator stack should be empty, and the value
   stack should have only one value in it, which is the final result.
*/
PyHandle PyLoad(char* code, int size)
{
	_context.src_code = code;
	_context.src_code_size = size;
	_context.spos = _context.src_code;
	std::cout << "Token->" << std::endl;

	std::stack<AST::Expression*> operands;
	std::stack<AST::Operator*> ops;

	std::vector<AST::Expression*> Lines;

	bool PreTokenIsOp = false;
	int pair_cnt = 0;//count for {} () and [],if
	//pair_cnt >0, eat new line
	while (true)
	{
		String s;
		short idx = GetToken(s);
		if (idx == TokenEOS)
		{
			break;
		}
		if (s.s == nil || s.size == 0)
		{
			continue;
		}
		if (idx == TokenID)
		{
			double dVal = 0;
			long long llVal = 0;
			ParseState st = ParseNumber(s, dVal, llVal);
			AST::Expression* v = nil;
			if (st == ParseState::Double)
			{
				idx = TokenNum;
				v = new AST::Double(dVal);
			}
			else if (st == ParseState::Long_Long)
			{
				idx = TokenNum;
				v = new AST::Number(llVal, s.size);
			}
			else
			{
				//Construct AST::Var
				v = new AST::Var(s);
			}
			operands.push(v);
			PreTokenIsOp = false;
		}
		else
		{
			std::string str(s.s, s.s + s.size);
			std::cout << "KW:" << idx << ",Val:" << str << std::endl;
			AST::Operator* op = nil;
			if (idx >= (short)KWIndex::Assign_Op_S 
				&& idx <= (short)KWIndex::Assign_Op_E)
			{
				op = new AST::Assign(idx);
			}
			else if (idx == (short)KWIndex::def)
			{
				op = new AST::Func();
			}
			else if (idx >= (short)KWIndex::Assign_Op_S
				&& idx <= (short)KWIndex::Assign_Op_E ||
				idx == (short)KWIndex::Dot)
			{
				op = new AST::BinaryOp(idx);
			}
			else if (idx == (short)KWIndex::Parenthesis_L)
			{
				pair_cnt++;
				op = new AST::Operator(idx);
			}
			else if (idx == (short)KWIndex::Parenthesis_R)
			{
				pair_cnt--;
				while (!ops.empty())
				{
					auto top = ops.top();
					if (top->getOp() == (short)KWIndex::Parenthesis_L)
					{
						ops.pop();
						break;
					}
					else
					{
						DoOpTop(operands, ops);
					}
				}
			}
			else if (idx == (short)KWIndex::Brackets_R)
			{

			}
			else if (idx == (short)KWIndex::Curlybracket_R)
			{

			}
			else if (idx == (short)KWIndex::Colon ||
				idx == (short)KWIndex::Comma)
			{//end block head
				op = new AST::Operator(idx);
			}
			else if (idx == (short)KWIndex::Add ||
				idx == (short)KWIndex::Minus || 
				idx == (short)KWIndex::Multiply)
			{
				if (PreTokenIsOp)
				{
					op = new AST::UnaryOp(idx);
				}
				else
				{
					op = new AST::BinaryOp(idx);
				}
			}
			else if (idx == (short)KWIndex::Invert)
			{
				if (PreTokenIsOp)
				{
					op = new AST::UnaryOp(idx);
				}
				else
				{
					//error
				}
			}
			else if (idx == (short)KWIndex::Slash)
			{
				op = new AST::Operator(idx);
			}
			else if (idx == (short)KWIndex::Newline)//end of code line
			{
				if ( pair_cnt>0 )
				{//line continue
					if (ops.top()->getOp() == (short)KWIndex::Slash)
					{
						delete ops.top();
						ops.pop();
					}
					continue;
				}
				else if (ops.top()->getOp() == (short)KWIndex::Slash)
				{//line continue
					delete ops.top();
					ops.pop();
					continue;
				}
				else if (ops.top()->getOp() == (short)KWIndex::Colon)
				{//end block head
					delete ops.top();
					ops.pop();
				}
				std::cout << "new Line" << std::endl;
				while (!ops.empty())
				{
					DoOpTop(operands, ops);
				}
				if (!operands.empty())
				{
					Lines.push_back(operands.top());
					operands.pop();
				}
			}
			if (op)
			{
				while (!ops.empty())
				{
					auto top = ops.top();
					if(Precedence(top->getOp())
						>/*=*/ Precedence(op->getOp()))
					//should use > not >+ fetch the right opernand for unary ops
					//for example x=-+-+10
					//but this calcluate right expresssion first
					//TODO:
					{
						DoOpTop(operands, ops);
					}
					else
					{
						break;
					}
				}
				ops.push(op);
			}
			if (idx == (short)KWIndex::Parenthesis_R)
			{
				PreTokenIsOp = false;
			}
			else
			{
				PreTokenIsOp = true;
			}
		}//end if (idx == TokenID)
	}//end while (true)
	while (!ops.empty())
	{
		DoOpTop(operands, ops);
	}
	if (!operands.empty())
	{
		Lines.push_back(operands.top());
		operands.pop();
	}
	for(auto l:Lines)
	{
		AST::Value v;
		bool  b = l->Run(v);
		b = b;
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

}