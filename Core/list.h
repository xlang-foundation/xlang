#pragma once
#include "object.h"
#include "str.h"
#include "scope.h"
#include "stackframe.h"
#include "xclass_object.h"

namespace X
{
namespace Data
{
class List :
	virtual public XList,
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
	List();
	List(std::vector<std::string>& strs) :
		List()
	{
		AutoLock autoLock(m_lock);
		for (auto& s : strs)
		{
			m_data.push_back(X::Value(new Str(s.c_str(), (int)s.size())));
		}
	}
	~List()
	{
		AutoLock autoLock(m_lock);
		m_bases.clear();
		m_ptrs.clear();
		m_data.clear();
	}
	inline virtual void AddItem(X::Value& v) override
	{
		Add(v);
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
	inline virtual bool GetIndexValue(int idx, Value& v) override
	{
		return Get(idx, v);
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
				proc(rt, pContext,idx, val, params, kwParams);
			}
		}
		else
		{
			for (size_t i = 0; i < m_data.size(); i++)
			{
				X::Value idx((int)i);
				proc(rt, pContext, idx,m_data[i], params, kwParams);
			}
		}
		return true;
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
		auto* newList = new List();
		newList->IncRef();
		auto size = Size();
		for (long long i = 0; i < size; i++)
		{
			Value v;
			Get(i, v);
			newList->Add(v);
		}
		return newList;
	}
	virtual List& operator +=(X::Value& r) override
	{
		AutoLock autoLock(m_lock);
		if (r.IsObject())
		{
			Object* pObj = dynamic_cast<Object*>(r.GetObj());
			if (pObj->GetType() == ObjType::List)
			{
				List* pOther = dynamic_cast<List*>(pObj);
				for (auto& it : pOther->m_data)
				{
					Add(nullptr,it);
				}
			}
			else
			{
				Add(nullptr,r);
			}
		}
		else
		{
			Add(nullptr, r);
		}

		return *this;
	}
	virtual bool ToBytes(XlangRuntime* rt,XObj* pContext,X::XLangStream& stream) override
	{
		AutoLock autoLock(m_lock);
		Object::ToBytes(rt,pContext,stream);
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
		//TODO:need to pass runtime,and calculate base class for some objects
		size_t size;
		stream >> size;
		for (size_t i = 0; i < size; i++)
		{
			X::Value v0;
			stream >> v0;
			m_data.push_back(v0);
		}
		return true;
	}
	virtual const char* ToString(bool WithFormat = false) override
	{
		AutoLock autoLock(m_lock);
		std::string strList = "[\n";
		size_t size = Size();
		for (size_t i = 0; i < size; i++)
		{
			X::Value v0;
			Get(i, v0);
			strList += '\t' + v0.ToString(WithFormat);
			if (i < (size - 1))
			{
				strList+=",\n";
			}
			else
			{
				strList += "\n";
			}
		}
		strList += "]";
		return GetABIString(strList);
	}
	inline virtual long long Size() override 
	{
		AutoLock autoLock(m_lock);
		return m_useLValue ? m_ptrs.size() : m_data.size();
	}
	inline void Clear()
	{
		AutoLock autoLock(m_lock);
		m_bases.clear();
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
		KWARGS& kwParams,X::Value& retValue) override;
	inline void Add(X::LValue p)
	{
		AutoLock autoLock(m_lock);
		m_useLValue = true;
		m_ptrs.push_back(p);
	}
	inline void Add(X::Value v)
	{
		AutoLock autoLock(m_lock);
		m_data.push_back(v);
	}
	inline void MakeCommonBases(
		AST::Expression* pThisBase,
		std::vector<Value>& bases_0)
	{
		if (m_bases.empty())//first item
		{//append all
			for (auto &it : bases_0)
			{
				if (it.IsObject())
				{
					Data::Object* pRealObj = dynamic_cast<Data::Object*>(it.GetObj());
					pRealObj->GetBaseScopes(m_bases);
				}
			}
			m_bases.push_back(dynamic_cast<AST::Scope*>(pThisBase));
		}
		else
		{//find common
			auto it = m_bases.begin();
			while (it != m_bases.end())
			{
				if (*it != dynamic_cast<AST::Scope*>(pThisBase))
				{
					bool bFind = false;
					for (auto it2 : bases_0)
					{
						if (*it == it2)
						{
							bFind = true;
							break;
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
	inline void Remove(long long idx)
	{
		AutoLock autoLock(m_lock);
		m_data.erase(std::next(m_data.begin(), idx));
	}
	inline void Insert(long long idx,XlangRuntime* rt, X::Value& v)
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
		m_data.insert(m_data.begin()+ idx,v);
	}
	inline void Add(XlangRuntime* rt, X::Value& v)
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
		m_data.push_back(v);
	}
	virtual List* FlatPack(XlangRuntime* rt, XObj* pContext,
		std::vector<std::string>& IdList, int id_offset,
		long long startIndex, long long count) override;
	virtual X::Value UpdateItemValue(XlangRuntime* rt, XObj* pContext,
		std::vector<std::string>& IdList, int id_offset,
		std::string itemName, X::Value& val) override;
	inline virtual bool Set(long long index, X::Value& v) override
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
	inline virtual bool GetAndUpdatePos(Iterator_Pos& pos, std::vector<Value>& vals) override
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
		pos = (Iterator_Pos)it;
		vals.push_back(val0);
		vals.push_back(X::Value(nPos));
		return true;
	}
	virtual Value Get(long long idx) override
	{
		Value v0;
		Get(idx, v0);
		return v0;
	}
	inline bool Get(long long idx, X::Value& v,
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
}
}
