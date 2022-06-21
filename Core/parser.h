#pragma once

#include "exp.h"
#include "token.h"
#include <stack>
#include <vector>
#include "def.h"
#include "block.h"
#include "blockstate.h"

namespace X {
enum class ParseState
{
	Wrong_Fmt,
	Null,
	Non_Number,
	Double,
	Long_Long
};

struct PairInfo
{
	int opid;
	bool IsLambda = false;
};
class Parser
{
	Token* mToken = nil;
	BlockState* m_curBlkState = nil;
	ParseState ParseHexBinOctNumber(String& str);
	ParseState ParseNumber(String& str, double& dVal, long long& llVal);

	inline bool LastIsLambda();
//for compile
	int m_lambda_pair_cnt = 0;
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
	std::vector<short> m_preceding_token_indexstack;

	AST::Block* m_lastComingBlock = nullptr;
	std::stack<BlockState*> m_stackBlocks;
	std::stack<PairInfo> m_stackPair;
private:
	void ResetForNewLine();
	void LineOpFeedIntoBlock(AST::Expression* line,
		AST::Indent& lineIndent);
public:
	void NewLine();
	AST::Operator* PairLeft(short opIndex);//For "(","[","{"
	void PairRight(OP_ID leftOpToMeetAsEnd); //For ')',']', and '}'
	inline void IncLambdaPairCnt() { m_lambda_pair_cnt++; }
	inline void DecLambdaPairCnt() { m_lambda_pair_cnt--; }
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
		return G::I().OpAct(idx);
	}
public:
	Parser();
	~Parser();
	bool Init();
	bool Compile(char* code, int size);
	bool Run();
};
}