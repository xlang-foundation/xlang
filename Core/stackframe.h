#pragma once
#include "def.h"
#include "value.h"

namespace X {namespace AST {
class Scope;
class StackFrame
{
protected:
	StackFrame* m_prev = nil;
	StackFrame* m_next = nil;
	Scope* m_pScope = nil;
	int m_varCnt = 0;
	Value* m_Values = nil;
	Value m_retVal;
public:
	StackFrame(Scope* s)
	{
		m_pScope = s;
	}
	~StackFrame()
	{
		if (m_Values)
		{
			delete[] m_Values;
		}
	}
	inline void SetNext(StackFrame* n) { m_next = n; if(n) n->m_prev = this; }
	inline void SetPrev(StackFrame* p) { m_prev = p; if(p) p->m_next = this; }
	inline StackFrame* Next() { return m_next; }
	inline StackFrame* Prev() { return m_prev; }
	inline bool belongTo(Scope* s) { return s == m_pScope; }
	void Copy(StackFrame* pFrom)
	{
		for (int i = 0; i < m_varCnt; i++)
		{
			m_Values[i] = pFrom->m_Values[i];
		}
		m_retVal = pFrom->m_retVal;
	}
	inline bool SetVarCount(int cnt)
	{
		if (cnt > 0)
		{
			m_Values = new Value[cnt];
			m_varCnt = cnt;
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