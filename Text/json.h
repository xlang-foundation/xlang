#pragma once

#include "token.h"

namespace X 
{
namespace Text 
{
struct JsonAction
{
	int precedence = 0;
};
struct JsonOpInfo
{
	int id;
	std::string name;
	JsonAction act;
};
class Json
{
	Token* mToken = nullptr;
	static std::vector<JsonOpInfo> OPList;
	static std::vector<short> kwTree;
	static std::vector<JsonAction> OpActions;
public:
	Json();
	~Json();
	bool Init();
	bool LoadFromString(char* code, int size);
	bool Parse();
};
}
}