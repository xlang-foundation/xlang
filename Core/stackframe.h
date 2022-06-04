#pragma once
#include "value.h"

namespace XPython {namespace AST {
class StackFrame
{
protected:
	Value* m_Values = nil;
	Value m_retVal;
public:
	StackFrame()
	{
	}
	~StackFrame()
	{
		if (m_Values)
		{
			delete[] m_Values;
		}
	}
	inline bool SetVarCount(int cnt)
	{
		m_Values =new Value(cnt);
		return true;
	}
	inline void Set(int idx, Value& v)
	{
		m_Values[idx] = v;
	}
	inline void SetReturn(Value& v)
	{
		m_retVal = v;
	}
	inline void Get(int idx, Value& v)
	{
		v = m_Values[idx];
	}
	inline Value& GetReturnValue()
	{
		return m_retVal;
	}
};
}
}