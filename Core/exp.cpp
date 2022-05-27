#include "exp.h"
#include <unordered_map>
#include <string>

namespace XPython {namespace AST {
static std::unordered_map < std::string, Value> _VarMap;

void Var::Set(Value& v)
{
	std::string key(Name.s, Name.size);
	_VarMap[key] = v;
}

bool Var::Run(Value& v)
{
	std::string key(Name.s, Name.size);
	auto it = _VarMap.find(key);
	if (it != _VarMap.end())
	{
		v = it->second;
		return true;
	}
	else
	{
		return false;
	}
}

}
}