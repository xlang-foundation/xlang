#pragma once

#include "exp.h"
#include "token.h"
#include <stack>
#include <vector>
#include "def.h"

namespace X {
enum class ParseState
{
	Wrong_Fmt,
	Null,
	Non_Number,
	Double,
	Long_Long
};

class Parser
{
	Token* mToken = nil;

	ParseState ParseHexBinOctNumber(String& str);
	ParseState ParseNumber(String& str, double& dVal, long long& llVal);
	inline void DoOpTop(std::stack<AST::Expression*>& operands,
		std::stack<AST::Operator*>& ops)
	{
		auto top = ops.top();
		ops.pop();
		top->OpWithOperands(operands);
	}

//for compile
	std::stack<AST::Expression*> m_operands;
	std::stack<AST::Operator*> m_ops;
	int m_pair_cnt = 0;//count for {} () and [],if
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
	//below,before meet first non-tab char,it is true 
	bool m_NewLine_WillStart = true;
	int m_TabCountAtLineBegin = 0;
	int m_LeadingSpaceCountAtLineBegin = 0;
	std::stack<AST::Block*> m_stackBlocks;
private:
	void ResetForNewLine();
public:
	void PushBlockStack(AST::Block* b)
	{
		m_stackBlocks.push(b);
	}
	void NewLine();
	AST::Operator* PairLeft(short opIndex);//For "(","[","{"
	void PairRight(OP_ID leftOpToMeetAsEnd); //For ')',']', and '}'
	inline void IncPairCnt() { m_pair_cnt++; }
	inline void DecPairCnt() { m_pair_cnt--; }
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
	inline void PushExp(AST::Expression* exp)
	{
		m_operands.push(exp);
	}
	inline void PushOp(AST::Operator* op)
	{
		m_ops.push(op);
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