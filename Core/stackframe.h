/*
Copyright (C) 2024 The XLang Foundation
Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

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
	//m_parent points to the parent stack frame with the callee thread
	//can be null 
	StackFrame* m_parent = nil;
	StackFrame* m_prev = nil;
	StackFrame* m_next = nil;

	Scope* m_pScope = nil;
	int m_varCnt = 0;
	X::Value* m_Values = nil;
	X::Value m_retVal;
	Expression* m_curExp = nullptr;
	int m_lineStart = -1;
	int m_charPos = 0;
#if XLANG_ENG_DBG
	void ObjDbgSet(XObj* pObj);
	void ObjDbgRemove(XObj* pObj);
#endif
public:
	FORCE_INLINE StackFrame()
	{
	}
	FORCE_INLINE StackFrame(Scope* s)
	{
		m_pScope = s;
	}
	FORCE_INLINE void SetScope(Scope* s)
	{
		m_pScope = s;
	}
	FORCE_INLINE void SetShareFlag(bool bShare)
	{
		m_bShared = bShare;
	}
	FORCE_INLINE ~StackFrame()
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
	FORCE_INLINE bool ToBytes(X::XLangStream& stream)
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
	FORCE_INLINE bool FromBytes(X::XLangStream& stream)
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
	FORCE_INLINE void SetCurExp(Expression* exp) { m_curExp = exp; }
	FORCE_INLINE Expression* GetCurExp() { return m_curExp; }
	FORCE_INLINE void SetCharPos(int c) { m_charPos = c; }
	FORCE_INLINE virtual int GetCharPos() { return m_charPos; }
	FORCE_INLINE Scope* GetScope() { return m_pScope; }
	FORCE_INLINE void SetParent(StackFrame* p) { m_parent = p; }
	FORCE_INLINE void SetNext(StackFrame* n) { m_next = n; if(n) n->m_prev = this; }
	FORCE_INLINE void SetPrev(StackFrame* p) { m_prev = p; if(p) p->m_next = this; }
	FORCE_INLINE StackFrame* Parent() { return m_parent; }
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
		if (idx < 0 || idx >= m_varCnt)
		{
			//TODO: just hack here, need to find why
			SetVarCount(idx+1);
			//std::cout << "StackFrame,Overflow,Var=" << m_varCnt << "Index="<<idx << std::endl;
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
		if ((idx < 0 && idx >= m_varCnt) || !m_Values)
		{
			if (m_bShared) m_lock.Unlock();
				return;
			//std::cout << "StackFrame,Overflow,Var=" << m_varCnt << "Index="<<idx << std::endl;
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