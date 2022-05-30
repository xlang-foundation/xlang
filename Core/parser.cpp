#include "parser.h"
#include <iostream>
#include <string>
#include <vector>


namespace XPython {	

std::vector<short> Parser::_kwTree;
std::vector<OpAction> Parser::OpActions;

ParseState Parser::ParseHexBinOctNumber(String& str)
{
	return ParseState::Null;
}
ParseState Parser::ParseNumber(String& str,
	double& dVal,long long& llVal)
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
			dVal = (double)primary[1];
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
	
Parser::Parser()
{
}

Parser::~Parser()
{
	if (mToken)
	{
		delete mToken;
	}
}

bool Parser::Init()
{
	MakeLexTree(OPList,_kwTree, OpActions);
	mToken = new Token(&_kwTree[0]);
	return true;
}
void Parser::DoOpTop(
	std::stack<AST::Expression*>& operands,
	std::stack<AST::Operator*>& ops)
{
	auto top = ops.top();
	auto al = OpAct(top->getOp()).alias;
	ops.pop();
	if ( al == Alias::Colon)
	{//:
		auto operandR = operands.top();
		operands.pop();
		auto operandL = operands.top();
		operands.pop();
		auto param = new AST::Param(operandL, operandR);
		operands.push(param);
	}
	else if (al == Alias::Comma)
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
bool Parser::Compile(char* code, int size)
{
	std::vector<AST::Expression*> Lines;

	mToken->SetStream(code, size);
	m_pair_cnt = 0;
	m_PreTokenIsOp = false;
	while (true)
	{
		String s;
		short idx = mToken->Get(s);
		if (idx == TokenEOS)
		{
			break;
		}
		if (idx == TokenStr)
		{
			AST::Str* v = new AST::Str(s.s, s.size);
			m_operands.push(v);
			m_PreTokenIsOp = false;
		}
		else if (idx == TokenID)
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
			m_operands.push(v);
			m_PreTokenIsOp = false;
		}
		else
		{//Operator
			m_PreTokenIsOp = true;//some action will change to false
			OpAction opAct = OpAct(idx);
			AST::Operator* op = nil;
			if (opAct.process)
			{
				op = opAct.process(this, idx, &opAct);
			}
			if (op)
			{
				while (!m_ops.empty())
				{
					auto top = m_ops.top();
					OpAction topAct = OpAct(top->getOp());
					OpAction opAct = OpAct(op->getOp());
					if (opAct.precedence> opAct.precedence)
					{
						DoOpTop(m_operands, m_ops);
					}
					else
					{
						break;
					}
				}
				m_ops.push(op);
			}
		}
	}
	while (!m_ops.empty())
	{
		DoOpTop(m_operands, m_ops);
	}
	if (!m_operands.empty())
	{
		Lines.push_back(m_operands.top());
		m_operands.pop();
	}
	for (auto l : Lines)
	{
		AST::Value v;
		bool  b = l->Run(v);
		b = b;
	}
	return true;
}

void Parser::NewLine()
{
	if (m_pair_cnt > 0)
	{//line continue
		if (m_ops.top()->getOp() == (short)KWIndex::Slash)
		{
			delete m_ops.top();
			m_ops.pop();
		}
		return;
	}
	else if (m_ops.top()->getOp() == (short)KWIndex::Slash)
	{//line continue
		delete m_ops.top();
		m_ops.pop();
		return;
	}
	else if (m_ops.top()->getOp() == (short)KWIndex::Colon)
	{//end block head
		delete m_ops.top();
		m_ops.pop();
	}
	while (!m_ops.empty())
	{
		DoOpTop(m_operands, m_ops);
	}
	if (!m_operands.empty())
	{
		//TODO:Lines.push_back(m_operands.top());
		m_operands.pop();
	}
}
void Parser::PairRight(Alias leftOpToMeetAsEnd)
{
	DecPairCnt();
	while (!m_ops.empty())
	{
		auto top = m_ops.top();
		if (OpAct(top->getOp()).alias == leftOpToMeetAsEnd)
		{
			m_ops.pop();
			break;
		}
		else
		{
			DoOpTop(m_operands, m_ops);
		}
	}
	m_PreTokenIsOp = false;
}
}