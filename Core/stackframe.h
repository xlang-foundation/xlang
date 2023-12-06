#pragma once
#include "def.h"
#include "value.h"
#include "lvalue.h"
#include "XLangStream.h"
#include <iostream>
#include "Locker.h"

namespace X {namespace AST {
class Scope;
class Expression;
class StackFrame
{
	Locker m_lock;
	//if this stack frame is shared by multiple threads,
	//will be set to true
	bool m_bShared = false;
protected:
	StackFrame* m_prev = nil;
	StackFrame* m_next = nil;
	Scope* m_pScope = nil;
	int m_varCnt = 0;
	X::Value* m_Values = nil;
	X::Value m_retVal;
	int m_lineStart = -1;
	int m_charPos = 0;
#if XLANG_ENG_DBG
	void ObjDbgSet(XObj* pObj);
	void ObjDbgRemove(XObj* pObj);
#endif
public:
	StackFrame()
	{
	}
	StackFrame(Scope* s)
	{
		m_pScope = s;
	}
	void SetShareFlag(bool bShare)
	{
		m_bShared = bShare;
	}
	~StackFrame()
	{
		if(m_bShared) m_lock.Lock();
		if (m_Values)
		{
#if XLANG_ENG_DBG
			for (int i = 0; i < m_varCnt; i++)
			{
				auto& v = m_Values[i];
				if (v.IsObject())
				{
					ObjDbgRemove(v.GetObj());
				}
			}
#endif
			delete[] m_Values;
		}
		if (m_bShared) m_lock.Unlock();
	}
	bool ToBytes(X::XLangStream& stream)
	{
		if (m_bShared) m_lock.Lock();
		stream << m_varCnt;
		for (int i = 0; i < m_varCnt; i++)
		{
			stream << m_Values[i];
		}
		if (m_bShared) m_lock.Unlock();
		return true;
	}
	bool FromBytes(X::XLangStream& stream)
	{
		if (m_bShared) m_lock.Lock();
		auto oldCnt = m_varCnt;
		stream >> m_varCnt;
		if (oldCnt != m_varCnt)
		{
			delete[] m_Values;
			m_Values = new X::Value[m_varCnt];
		}
		for (int i = 0; i < m_varCnt; i++)
		{
			stream >> m_Values[i];
		}
		if (m_bShared) m_lock.Unlock();
		return true;
	}
	bool AddVar(XlangRuntime* rt,std::string& name, X::Value& val);
	FORCE_INLINE virtual int GetStartLine() { return m_lineStart; }
	FORCE_INLINE void SetLine(int l) { m_lineStart = l; }
	FORCE_INLINE void SetCharPos(int c) { m_charPos = c; }
	FORCE_INLINE virtual int GetCharPos() { return m_charPos; }
	FORCE_INLINE Scope* GetScope() { return m_pScope; }
	FORCE_INLINE void SetNext(StackFrame* n) { m_next = n; if(n) n->m_prev = this; }
	FORCE_INLINE void SetPrev(StackFrame* p) { m_prev = p; if(p) p->m_next = this; }
	FORCE_INLINE StackFrame* Next() { return m_next; }
	FORCE_INLINE StackFrame* Prev() { return m_prev; }
	FORCE_INLINE bool belongTo(Scope* s) { return s == m_pScope; }
	void Copy(StackFrame* pFrom)
	{
		if (m_bShared) m_lock.Lock();
		for (int i = 0; i < m_varCnt; i++)
		{
			m_Values[i] = pFrom->m_Values[i];
		}
		m_retVal = pFrom->m_retVal;
		if (m_bShared) m_lock.Unlock();
	}
	FORCE_INLINE int GetVarCount() { return m_varCnt; }
	FORCE_INLINE bool SetVarCount(int cnt)
	{//can be called multiple times,
	//so need to check if m_Values is created
	//if created, copy data into new array
		if (m_bShared) m_lock.Lock();
		if (cnt == m_varCnt)
		{
			if (m_bShared) m_lock.Unlock();
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
		if (m_bShared) m_lock.Unlock();
		return true;
	}
	FORCE_INLINE void Set(int idx, X::Value& v)
	{
		if (m_bShared) m_lock.Lock();
		if (idx < 0 && idx >= m_varCnt)
		{
			std::cout << "StackFrame,Overflow,Var=" << m_varCnt << "Index="<<idx << std::endl;
		}
		m_Values[idx] = v;
#if XLANG_ENG_DBG
		if (v.IsObject())
		{
			ObjDbgSet(v.GetObj());
		}
#endif
		if (m_bShared) m_lock.Unlock();
	}
	FORCE_INLINE void SetReturn(X::Value& v)
	{
		if (m_bShared) m_lock.Lock();
		m_retVal = v;
		if (m_bShared) m_lock.Unlock();
	}
	FORCE_INLINE void Get(int idx, X::Value& v, X::LValue* lValue = nullptr)
	{
		if (m_bShared) m_lock.Lock();
		if (idx < 0 && idx >= m_varCnt)
		{
			std::cout << "StackFrame,Overflow,Var=" << m_varCnt << "Index="<<idx << std::endl;
		}
		X::Value& v0 = m_Values[idx];
		v = v0;
		if (lValue) *lValue = &v0;
		if (m_bShared) m_lock.Unlock();
	}
	FORCE_INLINE X::Value& GetReturnValue()
	{
		if (m_bShared)
		{
			AutoLock lock(m_lock);
			return m_retVal;
		}
		else
		{
			return m_retVal;
		}
	}
};
}
}