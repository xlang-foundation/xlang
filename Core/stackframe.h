#pragma once
#include "def.h"
#include "value.h"
#include "lvalue.h"

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
	X::Value* m_Values = nil;
	X::Value m_retVal;
	int m_lineStart = -1;
	int m_charPos = 0;
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
	inline virtual int GetStartLine() { return m_lineStart; }
	inline void SetLine(int l) { m_lineStart = l; }
	inline void SetCharPos(int c) { m_charPos = c; }
	inline virtual int GetCharPos() { return m_charPos; }
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
	inline int GetVarCount() { return m_varCnt; }
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
			X::Value* newList = new X::Value[cnt];
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
	inline void Set(int idx, X::Value& v)
	{
		if (idx < 0 && idx >= m_varCnt)
		{
			idx = idx;
		}
		m_Values[idx] = v;
	}
	inline void SetReturn(X::Value& v)
	{
		m_retVal = v;
	}
	inline void Get(int idx, X::Value& v, X::LValue* lValue = nullptr)
	{
		X::Value& v0 = m_Values[idx];
		v = v0;
		if (lValue) *lValue = &v0;
	}
	inline X::Value& GetReturnValue()
	{
		return m_retVal;
	}
};
}
}