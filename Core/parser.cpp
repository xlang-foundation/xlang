#include "parser.h"
#include <iostream>
#include <string>
#include <vector>
#include "action.h"
#include "runtime.h"

namespace X {	

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
	BuildOps();
	mToken = new Token(&G::I().GetKwTree()[0]);
	return true;
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
	mToken->SetStream(code, size);
	//mToken->Test();
	m_pair_cnt = 0;
	reset_preceding_token();
	//prepare top module for this code
	AST::Module* pTopModule = new AST::Module();
	pTopModule->ScopeLayout();
	m_stackBlocks.push(pTopModule);
	while (true)
	{
		String s;
		int leadingSpaceCnt = 0;
		OneToken one;
		short idx = mToken->Get(one);
		s = one.id;
		leadingSpaceCnt = one.leadingSpaceCnt;
		if (m_NewLine_WillStart)
		{
			m_LeadingSpaceCountAtLineBegin += leadingSpaceCnt;
		}
		if (idx == TokenEOS)
		{
			m_NewLine_WillStart = false;
			break;
		}
		if (idx == TokenLineComment || idx == TokenComment)
		{
		}
		else if (idx == TokenStr)
		{
			m_NewLine_WillStart = false;
			AST::Str* v = new AST::Str(s.s, s.size);
			m_operands.push(v);
			push_preceding_token(idx);
		}
		else if (idx == TokenID)
		{
			m_NewLine_WillStart = false;

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
			v->SetHint(one.lineStart, one.lineEnd, one.charPos);
			m_operands.push(v);
			push_preceding_token(idx);
		}
		else
		{//Operator
			OpAction opAct = OpAct(idx);
			if (m_NewLine_WillStart)
			{
				if (idx == G::I().GetOpId(OP_ID::Tab))
				{
					m_TabCountAtLineBegin++;
					continue;
				}
				else
				{
					m_NewLine_WillStart = false;
				}
			}
			AST::Operator* op = nil;
			if (opAct.process)
			{
				op = opAct.process(this, idx);
			}
			if (op)
			{
				op->SetHint(one.lineStart, one.lineEnd, one.charPos);
				while (!m_ops.empty())
				{
					auto top = m_ops.top();
					OpAction topAct = OpAct(top->getOp());
					OpAction cur_opAct = OpAct(op->getOp());
					short lastToken = get_last_token();
					//check this case .[test1,test2](....)
					//after . it is a ops,not var
					if (lastToken!=top->getOp() 
						&& top->m_type != AST::ObType::Pair 
						&& topAct.precedence> cur_opAct.precedence)
					{
						DoOpTop(m_operands, m_ops);
					}
					else
					{
						break;
					}
				}
				m_ops.push(op);
				push_preceding_token(idx);
			}
			else
			{//may meet ')',']','}', no OP here, already evaluated as an operand
				//push_preceding_token(TokenID);
			}
		}
	}
	NewLine();//just call it to process the last line
	while (m_stackBlocks.size() > 1)
	{
		m_stackBlocks.pop();//only keep top one
	}
	return true;
}

void Parser::ResetForNewLine()
{
	m_NewLine_WillStart = true;
	m_TabCountAtLineBegin = 0;
	m_LeadingSpaceCountAtLineBegin = 0;
}

void Parser::NewLine()
{
	if (!m_ops.empty())
	{
		auto top = m_ops.top();
		short topIdx = top->getOp();
		if (m_pair_cnt > 0)
		{//line continue
			if (topIdx == G::I().GetOpId(OP_ID::Slash))
			{
				delete top;
				m_ops.pop();
				pop_preceding_token();//pop Slash
			}
			return;
		}
		else if (topIdx == G::I().GetOpId(OP_ID::Slash))
		{//line continue
			delete top;
			m_ops.pop();
			pop_preceding_token();//pop Slash
			return;
		}
		else if (topIdx == G::I().GetOpId(OP_ID::Colon))
		{//end block head
			delete m_ops.top();
			m_ops.pop();
		}
		while (!m_ops.empty())
		{
			DoOpTop(m_operands, m_ops);
		}
	}
	if (!m_operands.empty() && !m_stackBlocks.empty())
	{
		auto line = m_operands.top();
		int startLine = line->GetStartLine();
		m_operands.pop();

		AST::Indent lineIndent = {
			m_TabCountAtLineBegin,m_LeadingSpaceCountAtLineBegin };
		auto curBlock = m_stackBlocks.top();
		auto indentCnt_CurBlock = curBlock->GetIndentCount();
		if (indentCnt_CurBlock < lineIndent)
		{
			auto child_indent_CurBlock = 
				curBlock->GetChildIndentCount();
			if (child_indent_CurBlock == AST::Indent{-1, -1})
			{
				curBlock->SetChildIndentCount(lineIndent);
				curBlock->Add(line);
			}
			else if (child_indent_CurBlock == lineIndent)
			{
				curBlock->Add(line);
			}
			else
			{
				//todo:error
			}
		}
		else
		{//go back to parent
			m_stackBlocks.pop();
			curBlock = nil;
			while (!m_stackBlocks.empty())
			{
				curBlock = m_stackBlocks.top();
				auto indentCnt = curBlock->GetChildIndentCount();
				//must already have child lines or block
				if (indentCnt == lineIndent)
				{
					break;
				}
				m_stackBlocks.pop();
			}
			if (curBlock!=nil)
			{
				curBlock->Add(line);
			}
			else
			{
				//TODO:: error
			}
		}
		//check if this is a block
		AST::Block* pValidBlock = dynamic_cast<AST::Block*>(line);
		if (pValidBlock)
		{
			pValidBlock->SetIndentCount(lineIndent);
			PushBlockStack(pValidBlock);
		}
	}
	ResetForNewLine();
}
void Parser::PairRight(OP_ID leftOpToMeetAsEnd)
{
	short lastToken = get_last_token();
	short pairLeftToken = G::I().GetOpId(leftOpToMeetAsEnd);
	bool bEmptyPair = (lastToken == pairLeftToken);
	DecPairCnt();
	AST::PairOp* pPair = nil;
	while (!m_ops.empty())
	{
		auto top = m_ops.top();
		if (top->getOp() == pairLeftToken)
		{
			pPair = (AST::PairOp*)top;
			m_ops.pop();
			break;
		}
		else
		{
			DoOpTop(m_operands, m_ops);
		}
	}
	if (!bEmptyPair)
	{
		if (!m_operands.empty() && pPair != nil)
		{
			pPair->SetR(m_operands.top());
			m_operands.pop();
		}
	}
	if (pPair)
	{
		//for case [](x,y), [] will change ('s PrecedingToken to TokenID
		if (pPair->GetPrecedingToken() == TokenID)
		{
			if (!m_operands.empty())
			{
				pPair->SetL(m_operands.top());
				m_operands.pop();
			}
		}
		m_operands.push(pPair);
	}
	//already evaluated as an operand
	push_preceding_token(TokenID);
}
AST::Operator* Parser::PairLeft(short opIndex)
{
	IncPairCnt();
	//case 1: x+(y+z), case 2: x+func(y,z)
	//so use preceding token as ref to dectect which case it is 
	short lastToken = get_last_token();
	auto op = new AST::PairOp(opIndex, lastToken);
#if 0
	if (!PreTokenIsOp())
	{//for case func(...),x[...] etc
		if (!m_operands.empty())
		{
			op->SetL(m_operands.top());
			m_operands.pop();
		}
	}
#endif
	return op;
}
bool Parser::Run()
{
	if (m_stackBlocks.empty())
	{
		return false;//empty
	}
	Runtime* pRuntime = new Runtime();
	AST::Module* pTopModule = (AST::Module*)m_stackBlocks.top();
	AST::StackFrame* frame = new AST::StackFrame(pTopModule);
	pRuntime->PushFrame(frame,pTopModule->GetVarNum());
	pRuntime->SetM(pTopModule);
	pTopModule->AddBuiltins(pRuntime);

	AST::Value v;
	bool bOK = pTopModule->Run(pRuntime,nullptr,v);
	pTopModule->PopFrame(pRuntime);
	delete frame;
	delete pTopModule;
	delete pRuntime;
	return bOK;
}
}