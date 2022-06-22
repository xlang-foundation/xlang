#include "parser.h"
#include <iostream>
#include <string>
#include <vector>
#include "action.h"
#include "runtime.h"
#include "module.h"
#include "var.h"
#include "func.h"
#include "http.h"
#include "manager.h"

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
	reset_preceding_token();
	//prepare top module for this code
	AST::Module* pTopModule = new AST::Module();
	pTopModule->ScopeLayout();
	BlockState* pBlockState=  new BlockState(pTopModule);
	m_stackBlocks.push(pBlockState);
	m_curBlkState = pBlockState;
	while (true)
	{
		String s;
		int leadingSpaceCnt = 0;
		OneToken one;
		short idx = mToken->Get(one);
		int startLine = one.lineStart;
		s = one.id;
		leadingSpaceCnt = one.leadingSpaceCnt;
		if (m_curBlkState->m_NewLine_WillStart)
		{
			m_curBlkState->m_LeadingSpaceCountAtLineBegin 
				+= leadingSpaceCnt;
		}
		if (idx == TokenEOS)
		{
			m_curBlkState->m_NewLine_WillStart = false;
			break;
		}
		if (idx == TokenLineComment || idx == TokenComment)
		{
		}
		else if (idx == TokenStr)
		{
			m_curBlkState->m_NewLine_WillStart = false;
			AST::Str* v = new AST::Str(s.s, s.size);
			v->SetHint(one.lineStart, one.lineEnd, one.charPos);
			m_curBlkState->PushExp(v);
			push_preceding_token(idx);
		}
		else if (idx == TokenID)
		{
			m_curBlkState->m_NewLine_WillStart = false;

			double dVal = 0;
			long long llVal = 0;
			ParseState st = ParseNumber(s, dVal, llVal);
			AST::Expression* v = nil;
			switch (st)
			{
			case X::ParseState::Double:
				idx = TokenNum;
				v = new AST::Double(dVal);
				break;
			case X::ParseState::Long_Long:
				idx = TokenNum;
				v = new AST::Number(llVal, s.size);
				break;
			default:
				//Construct AST::Var
				v = new AST::Var(s);
				break;
			}
			v->SetHint(one.lineStart, one.lineEnd, one.charPos);
			m_curBlkState->PushExp(v);
			push_preceding_token(idx);
		}
		else
		{//Operator
			OpAction opAct = OpAct(idx);
			if (m_curBlkState->m_NewLine_WillStart)
			{
				if (idx == G::I().GetOpId(OP_ID::Tab))
				{
					m_curBlkState->m_TabCountAtLineBegin++;
					continue;
				}
				else
				{
					m_curBlkState->m_NewLine_WillStart = false;
				}
			}
			AST::Operator* op = nil;
			if (opAct.process)
			{
				op = opAct.process(this, idx);
			}
			if (op)
			{
				auto pBlockOp = dynamic_cast<AST::Block*>(op);
				if (pBlockOp)
				{//will be used in NewLine function
					m_lastComingBlock = pBlockOp;
				}
				op->SetHint(one.lineStart, one.lineEnd, one.charPos);
				m_curBlkState->ProcessPrecedenceOp(
						get_last_token(), op);
				m_curBlkState->PushOp(op);
				push_preceding_token(idx);
			}
		}
	}
	NewLine();//just call it to process the last line
	while (m_stackBlocks.size() > 1)
	{
		auto top = m_stackBlocks.top();
		m_stackBlocks.pop();//only keep top one
		delete top;
	}
	return true;
}

void Parser::ResetForNewLine()
{
	m_curBlkState->m_NewLine_WillStart = true;
	m_curBlkState->m_TabCountAtLineBegin = 0;
	m_curBlkState->m_LeadingSpaceCountAtLineBegin = 0;
}
void Parser::LineOpFeedIntoBlock(AST::Expression* line,
	AST::Indent& lineIndent)
{
	auto* pCurBlockState = m_stackBlocks.top();
	auto curBlock = pCurBlockState->Block();
	auto indentCnt_CurBlock = 
		curBlock->GetIndentCount();
	if (curBlock->IsNoIndentCheck()
		|| indentCnt_CurBlock < lineIndent)
	{
		auto child_indent_CurBlock =
			curBlock->GetChildIndentCount();
		if (child_indent_CurBlock == AST::Indent{ 0,-1, -1 })
		{
			curBlock->SetChildIndentCount(lineIndent);
			curBlock->Add(line);
		}
		else if (child_indent_CurBlock == lineIndent)
		{
			curBlock->Add(line);
		}
		else if (curBlock->IsNoIndentCheck())
		{
			curBlock->Add(line);
		}
		else
		{
			//error
		}
	}
	else
	{//go back to parent
		delete m_stackBlocks.top();
		m_stackBlocks.pop();
		curBlock = nil;
		while (!m_stackBlocks.empty())
		{
			pCurBlockState = m_stackBlocks.top();
			curBlock = pCurBlockState->Block();
			auto indentCnt = 
				curBlock->GetChildIndentCount();
			//must already have child lines or block
			if (curBlock->IsNoIndentCheck()
				|| indentCnt == lineIndent)
			{
				m_curBlkState = pCurBlockState;
				break;
			}
			else
			{
				delete pCurBlockState;
				m_stackBlocks.pop();
			}
		}
		if (curBlock != nil)
		{
			curBlock->Add(line);
		}
		else
		{
			//TODO:: error
		}
	}
}
void Parser::NewLine(bool checkIfIsLambda)
{
	short lastToken = get_last_token();
	if (checkIfIsLambda && 
		lastToken == TokenID 
		&& LastIsLambda())
	{
		return;
	}
	if (!m_curBlkState->IsOpStackEmpty())
	{
		auto top = m_curBlkState->OpTop();
		short topIdx = top->getOp();
		if (m_curBlkState->PairCount() > 0 
			/* && m_lambda_pair_cnt == 0*/)
		{//line continue
			if (topIdx == G::I().GetOpId(OP_ID::Slash))
			{
				delete top;
				m_curBlkState->OpPop();
				pop_preceding_token();//pop Slash
			}
			return;
		}
		else if (topIdx == G::I().GetOpId(OP_ID::Slash))
		{//line continue
			delete top;
			m_curBlkState->OpPop();
			pop_preceding_token();//pop Slash
			return;
		}
		else if (topIdx == G::I().GetOpId(OP_ID::Colon))
		{//end block head
			delete top;
			m_curBlkState->OpPop();
		}
		while (!m_curBlkState->IsOpStackEmpty())
		{
			m_curBlkState->DoOpTop();
		}
	}
	if (!m_curBlkState->IsOperandStackEmpty()
		&& !m_stackBlocks.empty())
	{
		auto line = m_curBlkState->OperandTop();
		int startLine = line->GetStartLine();
		m_curBlkState->OperandPop();

		int leftMostCharPos = line->GetLeftMostCharPos();
		AST::Indent lineIndent = { leftMostCharPos,
			m_curBlkState->m_TabCountAtLineBegin,
			m_curBlkState->m_LeadingSpaceCountAtLineBegin };
		LineOpFeedIntoBlock(line, lineIndent);
		if(m_lastComingBlock)
		{
			auto pOpBlock = m_lastComingBlock;
			m_lastComingBlock = nullptr;
			pOpBlock->SetIndentCount(lineIndent);
			BlockState* pBlockState = new BlockState(pOpBlock);
			m_stackBlocks.push(pBlockState);
			m_curBlkState = pBlockState;
		}
	}
	ResetForNewLine();
}
void Parser::PairRight(OP_ID leftOpToMeetAsEnd)
{
	PairInfo pairInfo = m_stackPair.top();
	m_stackPair.pop();
	if (leftOpToMeetAsEnd == OP_ID::Curlybracket_L)
	{
		int cbId = G::I().GetOpId(OP_ID::Curlybracket_L);
		if (pairInfo.opid == cbId && pairInfo.IsLambda)
		{
			if(!m_stackBlocks.empty())
			{
				auto blk = m_stackBlocks.top();
				m_stackBlocks.pop();
				delete blk;
			}
			if (!m_stackBlocks.empty())
			{
				m_curBlkState = m_stackBlocks.top();
			}
			NewLine(false);
			//push_preceding_token(TokenID);
			DecLambdaPairCnt();
			m_curBlkState->DecPairCnt();
			return;
		}
	}
	short lastToken = get_last_token();
	short pairLeftToken = G::I().GetOpId(leftOpToMeetAsEnd);
	bool bEmptyPair = (lastToken == pairLeftToken);
	m_curBlkState->DecPairCnt();
	AST::PairOp* pPair = nil;
	while (!m_curBlkState->IsOpStackEmpty())
	{
		auto top = m_curBlkState->OpTop();
		if (top->getOp() == pairLeftToken)
		{
			pPair = (AST::PairOp*)top;
			m_curBlkState->OpPop();
			break;
		}
		else
		{
			m_curBlkState->DoOpTop();
		}
	}
	if (!bEmptyPair)
	{
		if (!m_curBlkState->IsOperandStackEmpty() 
			&& pPair != nil)
		{
			pPair->SetR(m_curBlkState->OperandTop());
			m_curBlkState->OperandPop();
		}
	}
	if (pPair)
	{
		//for case [](x,y), [] will change ('s PrecedingToken to TokenID
		if (pPair->GetPrecedingToken() == TokenID)
		{
			if (!m_curBlkState->IsOperandStackEmpty())
			{
				pPair->SetL(m_curBlkState->OperandTop());
				m_curBlkState->OperandPop();
			}
		}
		m_curBlkState->PushExp(pPair);
	}
	//already evaluated as an operand
	push_preceding_token(TokenID);
}
bool Parser::LastIsLambda()
{//check (...){ } pattern, but not like func(....){ }
	bool IsLambda = false;
	if (!m_curBlkState->IsOperandStackEmpty())
	{//check (...){ } pattern, but not like func(....){ }
		auto lastOpernad = m_curBlkState->OperandTop();
		if (lastOpernad->m_type == AST::ObType::Pair)
		{
			AST::PairOp* pPair = (AST::PairOp*)lastOpernad;
			if (pPair->getOp() == G::I().GetOpId(OP_ID::Parenthesis_L)
				&& pPair->GetL() == nil)
			{
				auto p_r = pPair->GetR();
				if (p_r)
				{
					if (p_r->m_type == AST::ObType::List)
					{
						IsLambda = true;
						auto& list = ((AST::List*)p_r)->GetList();
						for (auto i : list)
						{
							if (i->m_type != AST::ObType::Var &&
								i->m_type != AST::ObType::Param &&
								i->m_type != AST::ObType::Assign)
							{
								IsLambda = false;
								break;
							}
						}
					}
					else if (p_r->m_type == AST::ObType::Var ||
						p_r->m_type == AST::ObType::Param ||
						p_r->m_type == AST::ObType::Assign)
					{
						IsLambda = true;
					}
				}
			}
		}
	}
	return IsLambda;
}
AST::Operator* Parser::PairLeft(short opIndex)
{
	//case 1: x+(y+z), case 2: x+func(y,z)
	//so use preceding token as ref to dectect which case it is 
	short lastToken = get_last_token();
	if (lastToken == TokenID && LastIsLambda())
	{
		m_stackPair.push(PairInfo{ (int)opIndex,true });
		auto op = new AST::Func();
		op->NeedSetHint(true);
		op->SetNoIndentCheck(true);
		m_curBlkState->PushOp(op);
		push_preceding_token(opIndex);
		BlockState* pBlockState = new BlockState(op);
		m_stackBlocks.push(pBlockState);
		m_curBlkState = pBlockState;
		return nil;
	}
	else
	{
		m_stackPair.push(PairInfo{ (int)opIndex,false });
		m_curBlkState->IncPairCnt();
		auto op = new AST::PairOp(opIndex, lastToken);
		return op;
	}
}
bool Parser::Run()
{
	if (m_stackBlocks.empty())
	{
		return false;//empty
	}
	Runtime* pRuntime = new Runtime();
	BlockState* pBlockState = m_stackBlocks.top();
	AST::Module* pTopModule = (AST::Module*)pBlockState->Block();
	pRuntime->SetM(pTopModule);
	pTopModule->AddBuiltins(pRuntime);
	AST::Value v;
	bool bOK = pTopModule->Run(pRuntime, nullptr, v);
	delete pTopModule;
	delete pRuntime;
	return bOK;
}
}