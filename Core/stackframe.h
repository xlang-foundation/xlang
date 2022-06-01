#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include "value.h"

namespace XPython {namespace AST {
class StackFrame
{
protected:
	std::unordered_map < std::string, Value> m_VarMap;
	Value m_retVal;
public:
	StackFrame();
	~StackFrame();
	bool Have(std::string& name);
	bool Set(std::string& name, Value& v);
	bool SetReturn(Value& v);
	bool Get(std::string& name, Value& v);
	Value& GetReturnValue()
	{
		return m_retVal;
	}
};
}
}