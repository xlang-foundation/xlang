#include "parser.h"
#include <iostream>
#include <string>
#include "action.h"
#include "module.h"
#include "var.h"
#include "func.h"
#include "feedop.h"
#include "manager.h"
#include "op_registry.h"
#include "jitblock.h"
#include "jitlib.h"

namespace X
{
	extern XLoad* g_pXload;
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

bool Parser::Init(OpRegistry* reg)
{
	if (reg == nullptr)
	{
		m_reg = &G::I().R();
	}
	else
	{
		m_reg = reg;
	}
	mToken = new Token(&m_reg->GetKwTree()[0]);
	return true;
}

void Parser::ResetForNewLine()
{
	m_curBlkState->m_NewLine_WillStart = true;
	m_curBlkState->m_TabCountAtLineBegin = 0;
	m_curBlkState->m_LeadingSpaceCountAtLineBegin = 0;
}
bool Parser::LineOpFeedIntoBlock(AST::Expression* line,
	AST::Indent& lineIndent)
{
	if (m_bMeetJitBlock)
	{
		AST::Expression* pExpJitBlock = IsJitBlock(line);
		if (pExpJitBlock)
		{
			m_bInsideJitBlock = true;
			//reset it, we don't need this flag
			m_bMeetJitBlock = false;
			m_curJitBlock = (AST::JitBlock*)pExpJitBlock;
			m_blockStartCharPos = line->GetCharPos();
		}
	}
	if (m_stackBlocks.empty())
	{
		return false;
	}
	auto* pCurBlockState = m_stackBlocks.top();

	auto curBlock = pCurBlockState->Block();
	auto indentCnt_CurBlock = 
		curBlock->GetIndentCount();
	if (curBlock->IsNoIndentCheck()
		|| indentCnt_CurBlock < lineIndent)
	{
		static AST::Indent nullIndent{ 0, -1, -1 };
		auto child_indent_CurBlock =
			curBlock->GetChildIndentCount();
		if (child_indent_CurBlock.Equal(nullIndent))
		{
			curBlock->SetChildIndentCount(lineIndent);
			curBlock->Add(line);
			pCurBlockState->HaveNewLine(line);
		}
		else if (child_indent_CurBlock.Equal(lineIndent))
		{
			curBlock->Add(line);
			pCurBlockState->HaveNewLine(line);
		}
		else if (curBlock->IsNoIndentCheck())
		{
			curBlock->Add(line);
			pCurBlockState->HaveNewLine(line);
		}
		else
		{
			if (line->GetEndLine() > line->GetStartLine())
			{
				std::cout << "Line:" << line->GetStartLine()
					<< "-" << line->GetEndLine()
					<< "Error: line indent not match" << std::endl;
			}
			else
			{
				std::cout << "Line:" << line->GetStartLine()
					<< ", Compile Error: line indent not match" << std::endl;
			}
			return false;
		}
	}
	else
	{//go back to parent
		if (!m_stackBlocks.empty())
		{
			delete m_stackBlocks.top();
			m_stackBlocks.pop();
		}
		curBlock = nil;
		while (!m_stackBlocks.empty())
		{
			pCurBlockState = m_stackBlocks.top();
			curBlock = pCurBlockState->Block();
			auto indentCnt = 
				curBlock->GetChildIndentCount();
			//must already have child lines or block
			if (curBlock->IsNoIndentCheck()
				|| indentCnt.Equal(lineIndent))
			{
				m_curBlkState = pCurBlockState;
				break;
			}
			else
			{
				//for func or class, we need to re-calc line info bases on body's last item
				curBlock->ReCalcHintWithBody();
				delete pCurBlockState;
				m_stackBlocks.pop();
			}
		}
		if (curBlock != nil)
		{
			curBlock->Add(line);
			pCurBlockState->HaveNewLine(line);
		}
		else
		{
			std::cout << "Error: no block to add line" << std::endl;
			return false;
		}
	}
	return true;
}
bool Parser::NewLine(bool meetLineFeed_n,bool checkIfIsLambdaOrPair)
{
	if (meetLineFeed_n && m_curBlkState->m_SkipLineFeedN)
	{
		ResetForNewLine();
		return true;
	}
	//this case deals with { is in next line
	// f = (x,y)
	//{
	//...
	//}
	short lastToken = get_last_token();
	if (checkIfIsLambdaOrPair &&
		lastToken == TokenID 
		&& LastIsLambda())
	{
		return true;
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
			return true;
		}
		switch (topOpId)
		{
		case OP_ID::Slash:
			//line continue
			delete top;
			m_curBlkState->OpPop();
			pop_preceding_token();//pop Slash
			return true;
		case OP_ID::Colon:
			if (lastToken == topIdx)
			{
				delete top;
				m_curBlkState->OpPop();
			}
			break;
		case OP_ID::Comma:
			//todo:return;
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
		bool bOK = LineOpFeedIntoBlock(line, lineIndent);
		if (!bOK)
		{
			return false;
		}
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
	return true;
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
	short pairLeftToken = m_reg->GetOpId(leftOpToMeetAsEnd);
	bool bEmptyPair = (lastToken == pairLeftToken);
	AST::PairOp* pPair = nil;
	while (!m_curBlkState->IsOpStackEmpty())
	{
		auto top = m_curBlkState->OpTop();
		if (top->getOp() == pairLeftToken)
		{
			pPair = dynamic_cast<AST::PairOp*>(top);
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
			//loop all operands which's tokenIndex>pPair's tokenIndex
			//and only keep left-most
			AST::Expression* operandR = nullptr;
			while (!m_curBlkState->IsOperandStackEmpty()
				&& m_curBlkState->OperandTop()->GetTokenIndex() 
				> pPair->GetTokenIndex())
			{
				auto r = m_curBlkState->OperandTop();
				m_curBlkState->OperandPop();
				if (operandR == nullptr)
				{
					operandR = r;
				}
				else
				{
					delete operandR;
					operandR = r;
				}
			}
			pPair->SetR(operandR);
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
			AST::PairOp* pPair = dynamic_cast<AST::PairOp*>(lastOpernad);
			if (pPair->GetId() == OP_ID::Parenthesis_L
				&& pPair->GetL() == nil)
			{
				auto p_r = pPair->GetR();
				if (p_r)
				{
					if (p_r->m_type == AST::ObType::List)
					{
						IsLambda = true;
						auto& list = (dynamic_cast<AST::List*>(p_r))->GetList();
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
		//the last Operand is Pair, so set as R for this Func
		op->SetR(m_curBlkState->OperandTop());
		m_curBlkState->OperandPop();
		op->ScopeLayout();
		op->SetTokenIndex(m_tokenIndex++);
		//for code line inside this function, 
		//we need to assign upper block as its parent
		//it may be replaced 
		op->SetParent(m_curBlkState->Block());
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
		//out caller will set TokenIndex if op is not NULL
		return op;
	}
}

bool Parser::Compile(AST::Module* pModule,char* code, int size)
{
	char* szCode = pModule->SetCode(code, size);
	//keep memory to ref in AST tree
	mToken->SetStream(szCode, size);
	//mToken->Test();
	m_tokenIndex = 0;
	reset_preceding_token();
	BlockState* pBlockState = new BlockState(pModule);
	m_stackBlocks.push(pBlockState);
	m_curBlkState = pBlockState;
	//each expresion or op will get a tokenindex
	//which increased with the sequence come out from Token parser
	//use this way to make sure each op just get right operands

	X::Jit::JitLib* pJitLib = nullptr;

	while (true)
	{
		if (m_bInsideJitBlock)
		{
			mToken->SetSpecialPosToBeLessOrEqual(true, m_blockStartCharPos);
		}
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
		switch (idx)
		{
		case TokenSpecialPosToBeLessOrEqual:
		{
			//reset
			m_bInsideJitBlock = false;
			if (m_curJitBlock)
			{
				m_curJitBlock->SetJitCode(one.id);
				if (pJitLib == nullptr)
				{
					pJitLib = new X::Jit::JitLib(pModule->GetModuleName());
					std::string xlangEngPath = g_pXload->GetConfig().xlangEnginePath;
					pJitLib->SetXLangEngPath(xlangEngPath);
				}
				pJitLib->AddBlock(m_curJitBlock);
			}
		}
			break;
		case TokenLineComment:
		{
				AST::InlineComment* v = new AST::InlineComment(s.s, s.size);
				v->SetParent(pModule);
				v->SetHint(one.lineStart, one.lineEnd, one.charPos, one.charStart, one.charEnd);
				pModule->AddInlineComment(v);
		}
			break;
		case TokenFeedOp:
			{
				AST::Operator* op = new AST::FeedOp(s.s, s.size);
				op->SetTokenIndex(m_tokenIndex++);
				op->SetHint(one.lineStart, one.lineEnd, one.charPos, one.charStart, one.charEnd);
				m_curBlkState->ProcessPrecedenceOp(
					get_last_token(), op);
				m_curBlkState->PushOp(op);
				push_preceding_token(idx);
				bool bOKWithNewLine = NewLine(false);
				if (!bOKWithNewLine)
				{
					return false;
				}
			}
			break;
		case TokenComment:
		case TokenStr:
		case TokenStrWithFormat:
		case TokenCharSequence:
			{
				m_curBlkState->m_NewLine_WillStart = false;
				AST::Str* v = nullptr;
				if (idx == TokenComment)
				{//skip first """ and last """
					v = new AST::Str(s.s + 3, s.size - 6, idx == TokenStrWithFormat);
				}
				else
				{
					v = new AST::Str(s.s, s.size, idx == TokenStrWithFormat);
				}
				v->SetTokenIndex(m_tokenIndex++);
				v->SetCharFlag(idx == TokenCharSequence);
				v->SetHint(one.lineStart, one.lineEnd,
					one.charPos, one.charStart, one.charEnd);
				m_curBlkState->PushExp(v);
				push_preceding_token(idx);
			}
			break;
		case Token_False:
		case Token_True:
			{
				AST::Expression* v = new AST::Number(idx == Token_True);
				v->SetTokenIndex(m_tokenIndex++);
				v->SetHint(one.lineStart, one.lineEnd, one.charPos, one.charStart, one.charEnd);
				m_curBlkState->PushExp(v);
				push_preceding_token(TokenNum);
			}
			break;
		case Token_None:
			{
				AST::Expression* v = new AST::XConst((TokenIndex)idx);
				v->SetTokenIndex(m_tokenIndex++);
				v->SetHint(one.lineStart, one.lineEnd, one.charPos,
					one.charStart, one.charEnd);
				m_curBlkState->PushExp(v);
				push_preceding_token(Token_None);
			}
			break;
		case TokenID:
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
				case X::ParseState::Complex:
					idx = TokenNum;
					v = new AST::ImaginaryNumber(dVal);
					break;
				default:
					//Construct AST::Var
					v = new AST::Var(s);
					break;
				}
				v->SetTokenIndex(m_tokenIndex++);
				v->SetHint(one.lineStart, one.lineEnd, one.charPos, one.charStart, one.charEnd);
				m_curBlkState->PushExp(v);
				push_preceding_token(idx);
			}
			break;
		default:
			{//Operator
				OpAction opAct = OpAct(idx);
				if (m_curBlkState->m_NewLine_WillStart)
				{
					if (opAct.opId == OP_ID::Tab)
					{
						m_curBlkState->m_TabCountAtLineBegin++;
						continue;// while's continue
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
					op->SetTokenIndex(m_tokenIndex++);
					op->SetId(opAct.opId);
					auto pBlockOp = dynamic_cast<AST::Block*>(op);
					if (pBlockOp)
					{//will be used in NewLine function
						m_lastComingBlock = pBlockOp;
					}
					op->SetHint(one.lineStart, one.lineEnd, one.charPos, one.charStart, one.charEnd);
					m_curBlkState->ProcessPrecedenceOp(
						get_last_token(), op);
					m_curBlkState->PushOp(op);
					push_preceding_token(idx);
				}
			}//end default
			break;
		}//end switch
	}
	bool bOKWithNewLine = NewLine(false);//just call it to process the last line
	if (!bOKWithNewLine)
	{
		return false;
	}
	while (m_stackBlocks.size() > 1)
	{
		auto top = m_stackBlocks.top();
		m_stackBlocks.pop();//only keep top one
		delete top;
	}
	if (pJitLib)
	{
		pModule->SetJitLib(pJitLib);
		pJitLib->Build();
	}
	return true;
}

AST::Module* Parser::GetModule()
{
	AST::Module* pTopModule = nullptr;
	if (!m_stackBlocks.empty())
	{
		BlockState* pBlockState = m_stackBlocks.top();
		pTopModule = dynamic_cast<AST::Module*>(pBlockState->Block());
	}
	return pTopModule;
}

}