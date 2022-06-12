#pragma once
#include "value.h"

namespace X {namespace AST {
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
		if (cnt > 0)
		{
			m_Values = new Value[cnt];
		}
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
	inline void Get(int idx, Value& v, LValue* lValue = nullptr)
	{
		Value& v0 = m_Values[idx];
		v = v0;
		if (lValue) *lValue = &v0;
	}
	inline Value& GetReturnValue()
	{
		return m_retVal;
	}
};
}
}