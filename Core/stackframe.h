#pragma once
#include "def.h"
#include "value.h"

namespace X {namespace AST {
class Scope;
class Expression;
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
	StackFrame()
	{
	}
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
	inline Scope* GetScope() { return m_pScope; }
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
	{//can be called multiple times,
	//so need to check if m_Values is created
	//if created, copy data into new array
		if (cnt == m_varCnt)
		{
			return true;
		}
		if (cnt > 0)
		{
			Value* newList = new Value[cnt];
			if (m_Values)
			{
				for (int i = 0; i < cnt && i < m_varCnt; i++)
				{
					newList[i] = m_Values[i];
				}
				delete[] m_Values;
			}
			m_Values = newList;
			m_varCnt = cnt;
		}
		return true;
	}
	inline void Set(int idx, Value& v)
	{
		if (idx < 0 && idx >= m_varCnt)
		{
			idx = idx;
		}
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