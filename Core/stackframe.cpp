#include "stackframe.h"

namespace XPython {namespace AST {
StackFrame::StackFrame()
{

}
StackFrame::~StackFrame()
{

}
bool StackFrame::Have(std::string& name)
{
	auto it = m_VarMap.find(name);
	return (it != m_VarMap.end());
}
bool StackFrame::Set(std::string& name, Value& v)
{
	m_VarMap[name] = v;
	return true;
}
bool StackFrame::SetReturn(Value& v)
{
	m_retVal = v;
	return true;
}
bool StackFrame::Get(std::string& name, Value& v)
{
	bool bFind = false;
	auto it = m_VarMap.find(name);
	if (it != m_VarMap.end())
	{
		v = it->second;
		bFind = true;
	}
	return bFind;
}

}
}