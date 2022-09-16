#pragma once

#include "exp.h"
#include "token.h"
#include <stack>
#include <vector>
#include "def.h"
#include "block.h"
#include "blockstate.h"
#include "number.h"
#include "module.h"
#include "op_registry.h"

namespace X 
{
class OpRegistry;
class Parser
{
	OpRegistry* m_reg = nullptr;
	Token* mToken = nil;
	AST::Block* m_lastComingBlock = nullptr;
	std::stack<BlockState*> m_stackBlocks;
	BlockState* m_curBlkState = nil;
	std::vector<short> m_preceding_token_indexstack;

	inline bool LastIsLambda();
	//use this stack to keep 3 preceding tokens' index
	//and if meet slash, will pop one, because slash means line continuing
	inline void reset_preceding_token()
	{
		m_preceding_token_indexstack.clear();
	}
	inline void push_preceding_token(short idx)
	{
		if (m_preceding_token_indexstack.size() > 3)
		{
			m_preceding_token_indexstack.erase(
				m_preceding_token_indexstack.begin());
		}
		m_preceding_token_indexstack.push_back(idx);
	}
	inline short get_last_token()
	{
		return m_preceding_token_indexstack.size() > 0?
			m_preceding_token_indexstack[
				m_preceding_token_indexstack.size() - 1] :
			(short)TokenIndex::TokenInvalid;
	}
	inline void pop_preceding_token()
	{
		if (m_preceding_token_indexstack.size() > 0)
		{
			m_preceding_token_indexstack.pop_back();
		}
	}

private:
	void ResetForNewLine();
	void LineOpFeedIntoBlock(AST::Expression* line,
		AST::Indent& lineIndent);
public:
	BlockState* GetCurBlockState() {return m_curBlkState;}
	void NewLine(bool checkIfIsLambdaOrPair = true);
	AST::Operator* PairLeft(short opIndex);//For "(","[","{"
	void PairRight(OP_ID leftOpToMeetAsEnd); //For ')',']', and '}'
	inline bool PreTokenIsOp()
	{ 
		if (m_preceding_token_indexstack.size() == 0)
		{
			return false;
		}
		else
		{
			return m_preceding_token_indexstack[m_preceding_token_indexstack.size() - 1] >= 0;
		}
	}

	inline OpAction OpAct(short idx)
	{
		return G::I().R().OpAct(idx);
	}
public:
	Parser();
	~Parser();
	bool Init(OpRegistry* reg = nullptr);
	bool Compile(char* code, int size);
	AST::Module* GetModule();
};
}