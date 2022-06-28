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

namespace X
{		
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
void Parser::NewLine(bool checkIfIsLambdaOrPair)
{
	short lastToken = get_last_token();
	if (checkIfIsLambdaOrPair &&
		lastToken == TokenID 
		&& LastIsLambda())
	{
		return;
	}
	bool bInsideLambda = false;
	if (m_curBlkState->StackPair().size() > 0)
	{
		bInsideLambda = m_curBlkState->StackPair().top().IsLambda;
	}
	if (!m_curBlkState->IsOpStackEmpty())
	{
		auto top = m_curBlkState->OpTop();
		short topIdx = top->getOp();
		OP_ID topOpId = top->GetId();
		if (!bInsideLambda && m_curBlkState->StackPair().size() > 0)
		{//line continue
			if (topOpId == OP_ID::Slash)
			{
				delete top;
				m_curBlkState->OpPop();
				pop_preceding_token();//pop Slash
			}
			return;
		}
		switch (topOpId)
		{
		case OP_ID::Slash:
			//line continue
			delete top;
			m_curBlkState->OpPop();
			pop_preceding_token();//pop Slash
			return;
		case OP_ID::Colon:
			delete top;
			m_curBlkState->OpPop();
			break;
		case OP_ID::Comma:
			return;
		default:
			break;
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
	PairInfo pairInfo = m_curBlkState->StackPair().top();
	m_curBlkState->StackPair().pop();
	if (leftOpToMeetAsEnd == OP_ID::Curlybracket_L)
	{
		if (pairInfo.opId == OP_ID::Curlybracket_L 
			&& pairInfo.IsLambda)
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
			//Close Lambda
			m_curBlkState->DoOpTop();
			return;
		}
	}
	short lastToken = get_last_token();
	short pairLeftToken = G::I().GetOpId(leftOpToMeetAsEnd);
	bool bEmptyPair = (lastToken == pairLeftToken);
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
			if (pPair->GetId() == OP_ID::Parenthesis_L
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
				else
				{//no parameters
					IsLambda = true;
				}
			}
		}
	}
	return IsLambda;
}
AST::Operator* Parser::PairLeft(short opIndex)
{
	OpAction opAct = OpAct(opIndex);
	//case 1: x+(y+z), case 2: x+func(y,z)
	//so use preceding token as ref to dectect which case it is 
	short lastToken = get_last_token();
	if (lastToken == TokenID && LastIsLambda())
	{
		auto op = new AST::Func();
		op->NeedSetHint(true);
		op->SetNoIndentCheck(true);
		m_curBlkState->PushOp(op);
		push_preceding_token(opIndex);
		BlockState* pBlockState = new BlockState(op);
		m_stackBlocks.push(pBlockState);
		m_curBlkState = pBlockState;
		m_curBlkState->StackPair().push(PairInfo{ opAct.opId,(int)opIndex,true });
		return nil;
	}
	else
	{
		m_curBlkState->StackPair().push(PairInfo{ 
			opAct.opId,(int)opIndex,false });
		auto op = new AST::PairOp(opIndex, lastToken);
		return op;
	}
}

bool Parser::Compile(char* code, int size)
{
	mToken->SetStream(code, size);
	//mToken->Test();
	reset_preceding_token();
	//prepare top module for this code
	AST::Module* pTopModule = new AST::Module();
	pTopModule->SetCode(code, size);
	pTopModule->ScopeLayout();
	BlockState* pBlockState = new BlockState(pTopModule);
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
		//std::cout << startLine << ":" << std::string(s.s, s.size) << std::endl;
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
		else if (idx == TokenStr || idx == TokenStrWithFormat)
		{
			m_curBlkState->m_NewLine_WillStart = false;
			AST::Str* v = new AST::Str(s.s, s.size, idx == TokenStrWithFormat);
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
				if (opAct.opId == OP_ID::Tab)
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
				op->SetId(opAct.opId);
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
AST::Module* Parser::GetModule()
{
	AST::Module* pTopModule = nullptr;
	if (!m_stackBlocks.empty())
	{
		BlockState* pBlockState = m_stackBlocks.top();
		pTopModule = (AST::Module*)pBlockState->Block();
	}
	return pTopModule;
}

}