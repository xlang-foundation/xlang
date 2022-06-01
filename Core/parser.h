#pragma once

#include "exp.h"
#include "token.h"
#include <stack>
#include <vector>
#include "def.h"

namespace XPython {
class Parser;
struct OpAction;
typedef AST::Operator* (*OpProc)(
	Parser* p, short opIndex,OpAction* opAct);

class AList
{
public:
	AList()
	{

	}
	template<typename... As>
	AList(As... al)
	{
		m_list = std::vector <Alias>{ al... };
	}

	AList(Alias a)
	{
		m_list.push_back(a);
	}
	inline Alias operator[](int i)
	{
		return i>=(int)m_list.size()? Alias::None: m_list[i];
	}
private:
	std::vector <Alias> m_list;
};
#define Precedence_Reqular 1000
#define Precedence_Min 0

struct OpInfo
{
	std::vector<std::string> ops;
	OpProc process = nil;
	AList alias=Alias::None;
	int precedence = Precedence_Reqular;
};
struct OpAction
{
	OpProc process = nil;
	Alias alias = Alias::None;
	int precedence = Precedence_Reqular;
};
enum class ParseState
{
	Wrong_Fmt,
	Null,
	Non_Number,
	Double,
	Long_Long
};

void MakeLexTree(std::vector<OpInfo>& opList,
	std::vector<short>& buffer,
	std::vector<OpAction>& opActions);
class Parser
{
	static std::vector<short> _kwTree;
	static std::vector<OpInfo> OPList;
	static std::vector<OpAction> OpActions;
	Token* mToken = nil;

	ParseState ParseHexBinOctNumber(String& str);
	ParseState ParseNumber(String& str, double& dVal, long long& llVal);
	void DoOpTop(std::stack<AST::Expression*>& operands,
		std::stack<AST::Operator*>& ops);

//for compile
	std::stack<AST::Expression*> m_operands;
	std::stack<AST::Operator*> m_ops;
	int m_pair_cnt = 0;//count for {} () and [],if
	bool m_PreTokenIsOp = false;
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
	AST::Operator* PairLeft(short opIndex,OpAction* opAct);//For "(","[","{"
	void PairRight(Alias leftOpToMeetAsEnd); //For ')',']', and '}'
	inline void IncPairCnt() { m_pair_cnt++; }
	inline bool PreTokenIsOp() { return m_PreTokenIsOp; }
	inline void DecPairCnt() { m_pair_cnt--; }
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
		return (idx>=0 && idx< (short)OpActions.size()) ?
			OpActions[idx]:OpAction();
	}
public:
	Parser();
	~Parser();
	bool Init();
	bool Compile(char* code, int size);
	bool Run();
};
}