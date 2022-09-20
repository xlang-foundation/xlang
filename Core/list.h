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
class ListScope :
	virtual public AST::Scope
{
	List* m_owner = nullptr;
	AST::StackFrame* m_stackFrame = nullptr;
public:
	ListScope(List* p) :
		Scope()
	{
		m_owner = p;
	}
	~ListScope()
	{
		if (m_stackFrame)
		{
			delete m_stackFrame;
		}
	}
	void Init();
	// Inherited via Scope
	virtual Scope* GetParentScope() override;
	virtual bool Set(Runtime* rt, XObj* pContext, int idx, Value& v) override
	{
		m_stackFrame->Set(idx, v);
		return true;
	}
	virtual bool Get(Runtime* rt, XObj* pContext, int idx, Value& v,
		LValue* lValue = nullptr) override
	{
		m_stackFrame->Get(idx, v, lValue);
		return true;
	}
};
class List :
	public virtual Object
{
	friend class ListScope;
protected:
	bool m_useLValue = false;
	std::vector<X::Value> m_data;
	std::vector<X::LValue> m_ptrs;
	std::vector<AST::Scope*> m_bases;
	ListScope* m_listScope = nullptr;
public:
	List() :
		Object()
	{
		m_t = ObjType::List;
		m_listScope = new ListScope(this);
		m_listScope->Init();
		m_listScope->AddRef();
		m_bases.push_back(m_listScope);

	}
	List(std::vector<std::string>& strs) :
		List()
	{
		AutoLock(m_lock);
		for (auto& s : strs)
		{
			m_data.push_back(X::Value(new Str(s.c_str(), (int)s.size())));
		}
	}
	~List()
	{
		AutoLock(m_lock);
		m_bases.clear();
		m_ptrs.clear();
		m_data.clear();
		m_listScope->Release();
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
		IterateProc proc, ARGS& params, KWARGS& kwParams) override
	{
		AutoLock(m_lock);
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
		AutoLock(m_lock);
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
	virtual List& operator +=(X::Value& r) override
	{
		AutoLock(m_lock);
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
	virtual bool ToBytes(Runtime* rt,XObj* pContext,X::XLangStream& stream) override
	{
		AutoLock(m_lock);
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
		AutoLock(m_lock);
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
	virtual std::string ToString(bool WithFormat = false) override
	{
		AutoLock(m_lock);
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
		return strList;
	}
	inline virtual long long Size() override 
	{
		AutoLock(m_lock);
		return m_useLValue ? m_ptrs.size() : m_data.size();
	}
	ARGS& Data()
	{
		return m_data;
	}
	virtual void GetBaseScopes(std::vector<AST::Scope*>& bases) override
	{ 
		for (auto it : m_bases)
		{
			bases.push_back(it);
		}
	}
	virtual bool Call(XRuntime* rt, XObj* pContext, ARGS& params,
		KWARGS& kwParams,X::Value& retValue) override;
	inline void Add(X::LValue p)
	{
		AutoLock(m_lock);
		m_useLValue = true;
		m_ptrs.push_back(p);
	}
	inline void MakeCommonBases(
		AST::Expression* pThisBase,
		std::vector<AST::XClass*>& bases_0)
	{
		if (m_bases.empty())//first item
		{//append all
			for (auto it : bases_0)
			{
				m_bases.push_back(dynamic_cast<AST::Scope*>(it));
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
		AutoLock(m_lock);
		m_data.erase(std::next(m_data.begin(), idx));
	}
	inline void Add(Runtime* rt, X::Value& v)
	{
		AutoLock(m_lock);
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
				std::vector<AST::XClass*> dummy;
				MakeCommonBases(rt->M(), dummy);
			}
		}
		m_data.push_back(v);
	}
	virtual List* FlatPack(Runtime* rt,long long startIndex, long long count) override;
	inline bool Get(long long idx, X::Value& v,
		X::LValue* lValue = nullptr)
	{
		AutoLock(m_lock);
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
