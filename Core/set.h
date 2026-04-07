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
#include "object.h"
#include "str.h"
#include "scope.h"
#include "stackframe.h"
#include "xclass_object.h"
#include "function.h"

namespace X
{
namespace Data
{
class XlangSet :
	virtual public XSet,
	virtual public Object
{
protected:
	bool m_useLValue = false;
	std::vector<X::Value> m_data;
	std::vector<X::LValue> m_ptrs;
	std::vector<AST::Scope*> m_bases;
public:
	static void Init();
	static void cleanup();
	XlangSet();
	XlangSet(std::vector<std::string>& strs) :
		XlangSet()
	{
		AutoLock autoLock(m_lock);
		for (auto& s : strs)
		{
			m_data.push_back(X::Value(new Str(s.c_str(), (int)s.size())));
		}
	}
	XlangSet(X::ARGS& params) :
		XlangSet()
	{
		AutoLock autoLock(m_lock);
		for (auto& param : params)
		{
			AddUnique(param);
		}
	}
	~XlangSet()
	{
		AutoLock autoLock(m_lock);
		m_bases.clear();
		m_ptrs.clear();
		m_data.clear();
	}
	template<typename T>
	std::vector<T> Map(EnumProc proc)
	{
		std::vector<T> outs;
		if (m_useLValue)
		{
			for (size_t i = 0; i < m_ptrs.size(); i++)
			{
				X::Value v = proc(*m_ptrs[i], i);
				outs.push_back((T)v);
			}
		}
		else
		{
			for (size_t i = 0; i < m_data.size(); i++)
			{
				X::Value v = proc(m_data[i], i);
				outs.push_back((T)v);
			}
		}
		return outs;
	}
	virtual bool Iterate(X::XRuntime* rt, XObj* pContext,
		IterateProc proc, ARGS& params, KWARGS& kwParams,
		X::Value& retValue) override
	{
		AutoLock autoLock(m_lock);
		if (m_useLValue)
		{
			for (size_t i = 0; i < m_ptrs.size(); i++)
			{
				X::Value idx((int)i);
				X::Value val(*m_ptrs[i]);
				proc(rt, pContext, idx, val, params, kwParams);
			}
		}
		else
		{
			for (size_t i = 0; i < m_data.size(); i++)
			{
				X::Value idx((int)i);
				proc(rt, pContext, idx, m_data[i], params, kwParams);
			}
		}
		return true;
	}
	virtual bool IsContain(X::Value& val) override
	{
		AutoLock autoLock(m_lock);
		if (m_useLValue)
		{
			for (size_t i = 0; i < m_ptrs.size(); i++)
			{
				if (val == (X::Value)(*m_ptrs[i]))
				{
					return true;
				}
			}
		}
		else
		{
			for (size_t i = 0; i < m_data.size(); i++)
			{
				if (val == (X::Value)(m_data[i]))
				{
					return true;
				}
			}
		}
		return false;
	}
	void Each(EnumProc proc)
	{
		AutoLock autoLock(m_lock);
		if (m_useLValue)
		{
			for (size_t i = 0; i < m_ptrs.size(); i++)
			{
				Value val(*m_ptrs[i]);
				proc(val, i);
			}
		}
		else
		{
			for (size_t i = 0; i < m_data.size(); i++)
			{
				proc(m_data[i], i);
			}
		}
	}
	virtual XObj* Clone() override
	{
		auto* newSet = new XlangSet();
		newSet->IncRef();
		auto size = Size();
		for (long long i = 0; i < size; i++)
		{
			Value v;
			Get(i, v);
			newSet->AddUnique(v);
		}
		return newSet;
	}

	// += operator: in-place union (Python set |= / update semantics)
	virtual XlangSet& operator +=(X::Value& r) override
	{
		AutoLock autoLock(m_lock);
		if (r.IsObject())
		{
			Object* pObj = dynamic_cast<Object*>(r.GetObj());
			if (pObj->GetType() == ObjType::Set)
			{
				XlangSet* pOther = dynamic_cast<XlangSet*>(pObj);
				for (auto& it : pOther->m_data)
				{
					X::Value v = it;
					AddUnique_NoLock(v);
				}
			}
			else
			{
				AddUnique_NoLock(r);
			}
		}
		else
		{
			AddUnique_NoLock(r);
		}
		return *this;
	}

	// -= operator: in-place difference
	XlangSet& operator -=(X::Value& r)
	{
		AutoLock autoLock(m_lock);
		if (r.IsObject())
		{
			Object* pObj = dynamic_cast<Object*>(r.GetObj());
			if (pObj->GetType() == ObjType::Set)
			{
				XlangSet* pOther = dynamic_cast<XlangSet*>(pObj);
				for (auto& it : pOther->m_data)
				{
					X::Value v = it;
					Remove_NoLock(v);
				}
			}
			else
			{
				Remove_NoLock(r);
			}
		}
		else
		{
			Remove_NoLock(r);
		}
		return *this;
	}

	virtual bool ToBytes(XlangRuntime* rt, XObj* pContext, X::XLangStream& stream) override
	{
		AutoLock autoLock(m_lock);
		Object::ToBytes(rt, pContext, stream);
		size_t size = Size();
		stream << size;
		for (size_t i = 0; i < size; i++)
		{
			X::Value v0;
			Get(i, v0);
			stream << v0;
		}
		return true;
	}
	virtual bool FromBytes(X::XLangStream& stream) override
	{
		AutoLock autoLock(m_lock);
		size_t size;
		stream >> size;
		for (size_t i = 0; i < size; i++)
		{
			X::Value v0;
			stream >> v0;
			AddUnique_NoLock(v0);
		}
		return true;
	}
	// Python-style compact: {1, 2, 3}
	virtual const char* ToString(bool WithFormat = false) override
	{
		AutoLock autoLock(m_lock);
		size_t size = Size();
		if (size == 0)
		{
			std::string emptySet("set()");
			return GetABIString(emptySet);
		}
		std::string strSet = "{";
		for (size_t i = 0; i < size; i++)
		{
			X::Value v0;
			Get(i, v0);
			strSet += v0.ToString(WithFormat);
			if (i < (size - 1))
			{
				strSet += ", ";
			}
		}
		strSet += "}";
		return GetABIString(strSet);
	}
	FORCE_INLINE virtual long long Size() override
	{
		AutoLock autoLock(m_lock);
		return m_useLValue ? m_ptrs.size() : m_data.size();
	}
	FORCE_INLINE void Clear()
	{
		AutoLock autoLock(m_lock);
		m_ptrs.clear();
		m_data.clear();
	}
	std::vector<X::Value>& Data()
	{
		return m_data;
	}
	virtual void GetBaseScopes(std::vector<AST::Scope*>& bases) override
	{
		Object::GetBaseScopes(bases);
		for (auto it : m_bases)
		{
			bases.push_back(it);
		}
	}
	virtual bool Call(XRuntime* rt, XObj* pContext, ARGS& params,
		KWARGS& kwParams, X::Value& retValue) override;

	// Internal: Add element without checking for duplicate (caller holds lock)
	FORCE_INLINE void AddUnique_NoLock(X::Value& v)
	{
		for (size_t i = 0; i < m_data.size(); i++)
		{
			if (v == (X::Value)(m_data[i]))
			{
				return; // already exists
			}
		}
		m_data.push_back(v);
	}

	// Internal: Remove by value without holding lock (caller holds lock)
	FORCE_INLINE bool Remove_NoLock(X::Value& v)
	{
		for (size_t i = 0; i < m_data.size(); i++)
		{
			if (v == (X::Value)(m_data[i]))
			{
				m_data.erase(std::next(m_data.begin(), i));
				return true;
			}
		}
		return false;
	}

	// Public add: acquire lock, deduplicate
	FORCE_INLINE void Add(X::LValue p)
	{
		AutoLock autoLock(m_lock);
		bool isDup = false;
		for (size_t i = 0; i < m_ptrs.size(); i++)
		{
			if (p == (X::LValue)(*m_ptrs[i])) {
				isDup = true;
				break;
			}
		}
		m_useLValue = true;
		if (!isDup)
			m_ptrs.push_back(p);
	}
	FORCE_INLINE void Add(X::Value v)
	{
		AutoLock autoLock(m_lock);
		AddUnique_NoLock(v);
	}
	// Thread-safe public add (unique)
	FORCE_INLINE void AddUnique(X::Value& v)
	{
		AutoLock autoLock(m_lock);
		AddUnique_NoLock(v);
	}
	FORCE_INLINE void Add(XlangRuntime* rt, X::Value& v)
	{
		AutoLock autoLock(m_lock);
		if (v.IsObject())
		{
			Object* obj = dynamic_cast<Object*>(v.GetObj());
			if (obj->GetType() == ObjType::XClassObject)
			{
				XClassObject* pClassObj = dynamic_cast<XClassObject*>(obj);
				if (pClassObj)
				{
					AST::XClass* pXClass = pClassObj->GetClassObj();
					if (pXClass)
					{
						auto& bases_0 = pXClass->GetBases();
						MakeCommonBases(pXClass, bases_0);
					}
				}
			}
			else if (obj->GetType() == ObjType::Function)
			{
				std::vector<Value> dummy;
				MakeCommonBases(rt->M(), dummy);
			}
		}
		AddUnique_NoLock(v);
	}

	// Remove by index (legacy support)
	FORCE_INLINE void Remove(long long idx)
	{
		AutoLock autoLock(m_lock);
		if (!m_useLValue)
		{
			if (idx >= 0 && idx < (long long)m_data.size())
			{
				m_data.erase(std::next(m_data.begin(), idx));
			}
		}
		else
		{
			if (idx >= 0 && idx < (long long)m_ptrs.size())
			{
				m_ptrs.erase(std::next(m_ptrs.begin(), idx));
			}
		}
	}

	// Remove by value — returns true if found and removed
	FORCE_INLINE bool Remove(X::Value& v)
	{
		AutoLock autoLock(m_lock);
		return Remove_NoLock(v);
	}
	FORCE_INLINE void Remove(XlangRuntime* rt, X::Value& v)
	{
		AutoLock autoLock(m_lock);
		Remove_NoLock(v);
	}

	FORCE_INLINE void MakeCommonBases(
		AST::Expression* pThisBase,
		std::vector<Value>& bases_0)
	{
		if (m_bases.empty())//first item
		{//append all
			for (auto& it : bases_0)
			{
				if (it.IsObject())
				{
					Data::Object* pRealObj = dynamic_cast<Data::Object*>(it.GetObj());
					pRealObj->GetBaseScopes(m_bases);
				}
			}
			auto* pBaseScope = pThisBase->GetMyScope();
			if (pBaseScope)
			{
				m_bases.push_back(pBaseScope);
			}
		}
		else
		{//find common
			auto it = m_bases.begin();
			while (it != m_bases.end())
			{
				if (*it != pThisBase->GetMyScope())
				{
					bool bFind = false;
					for (auto it2 : bases_0)
					{
						if (it2.IsObject())
						{
							auto* pXObj = it2.GetObj();
							if (pXObj->GetType() == ObjType::XClassObject)
							{
								XClassObject* pClassObj = dynamic_cast<XClassObject*>(pXObj);
								if (pClassObj)
								{
									AST::XClass* pXClass = pClassObj->GetClassObj();
									if (pXClass)
									{
										auto* pBaseScope = pXClass->GetMyScope();
										if (*it == pBaseScope)
										{
											bFind = true;
											break;
										}
									}
								}
							}
							else if (pXObj->GetType() == ObjType::Function)
							{
								Function* pFunc = dynamic_cast<Function*>(pXObj);
								if (pFunc)
								{
									auto* pBaseScope = pFunc->GetMyScope();
									if (*it == pBaseScope)
									{
										bFind = true;
										break;
									}
								}
							}
						}
					}//end for
					if (!bFind)
					{
						it = m_bases.erase(it);
						continue;
					}
				}
				++it;
			}//end while
		}//end else
	}
	virtual List* FlatPack(XlangRuntime* rt, XObj* pContext,
		std::vector<std::string>& IdSet, int id_offset,
		long long startIndex, long long count) override;
	FORCE_INLINE bool SetVal(X::Value& v)
	{
		AutoLock autoLock(m_lock);
		if (m_useLValue)
			m_ptrs.push_back(v);
		else
			AddUnique_NoLock(v);
		return true;
	}
	FORCE_INLINE virtual bool Set(long long index, X::Value& v) override
	{
		AutoLock autoLock(m_lock);
		if (m_useLValue)
		{
			X::LValue l = m_ptrs[index];
			if (l)
			{
				*l = v;
			}
		}
		else
		{
			m_data[index] = v;
		}
		return true;
	}
	FORCE_INLINE virtual bool GetAndUpdatePos(Iterator_Pos& pos,
		std::vector<Value>& vals, bool getOnly) override
	{
		long long it = (long long)pos;
		X::Value val0;
		long long nPos = it;
		AutoLock autoLock(m_lock);
		if (m_useLValue)
		{
			if (it >= (long long)m_ptrs.size())
			{
				return false;
			}
			X::LValue l = m_ptrs[it++];
			if (l)
			{
				val0 = *l;
			}
		}
		else
		{
			if (it >= (long long)m_data.size())
			{
				return false;
			}
			val0 = m_data[it++];
		}
		if (!getOnly)
		{
			pos = (Iterator_Pos)it;
		}
		vals.push_back(val0);
		vals.push_back(X::Value(nPos));
		return true;
	}
	FORCE_INLINE bool Get(long long idx, X::Value& v,
		X::LValue* lValue = nullptr)
	{
		AutoLock autoLock(m_lock);
		if (m_useLValue)
		{
			if (idx >= (long long)m_ptrs.size())
			{
				return false;
			}
			X::LValue l = m_ptrs[idx];
			if (l)
			{
				v = *l;
			}
			if (lValue) *lValue = l;
		}
		else
		{
			if (idx >= (long long)m_data.size())
			{
				m_data.resize(idx + 1);
			}
			X::Value& v0 = m_data[idx];
			v = v0;
			if (lValue) *lValue = &v0;
		}
		return true;
	}
};

// Forward alias: xlang and new code can use Set = XlangSet
using Set = XlangSet;

}
}
