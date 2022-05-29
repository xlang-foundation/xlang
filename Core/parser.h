#pragma once

#include "exp.h"
#include "token.h"
#include <stack>

namespace XPython {
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
	void DoOpTop(std::stack<AST::Expression*>& operands,
		std::stack<AST::Operator*>& ops);

	static int _precedence[(short)KWIndex::MaxCount];
	inline int Precedence(short idx)
	{
		return _precedence[idx];
	}

public:
	Parser();
	~Parser();
	bool Init(short* kwTree);
	bool Compile(char* code, int size);
};
}