#pragma once
#include "object.h"
#include "str.h"

namespace X
{
namespace Data
{
class List :
	public Object
{
protected:
	bool m_useLValue = false;
	std::vector<X::Value> m_data;
	std::vector<X::LValue> m_ptrs;
	std::vector<AST::Expression*> m_bases;
public:
	List() :
		Object()
	{
		m_t = ObjType::List;

	}
	List(std::vector<std::string>& strs) :
		List()
	{
		for (auto& s : strs)
		{
			m_data.push_back(X::Value(new Str(s.c_str(), (int)s.size())));
		}
	}
	~List()
	{
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
	virtual bool Iterate(X::XRuntime* rt, void* pContext,
		IterateProc proc, ARGS& params, KWARGS& kwParams) override
	{
		if (m_useLValue)
		{
			for (size_t i = 0; i < m_ptrs.size(); i++)
			{
				X::Value idx((int)i);
				proc(rt, pContext,idx,* m_ptrs[i], params, kwParams);
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
		if (m_useLValue)
		{
			for (size_t i = 0; i < m_ptrs.size(); i++)
			{
				proc(*m_ptrs[i], i);
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
		if (r.IsObject())
		{
			Object* pObj = dynamic_cast<Object*>(r.GetObj());
			if (pObj->GetType() == ObjType::List)
			{
				List* pOther = (List*)pObj;
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
	virtual bool ToBytes(X::XLangStream& stream) override
	{
		Object::ToBytes(stream);
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
		return m_useLValue ? m_ptrs.size() : m_data.size(); 
	}
	ARGS& Data()
	{
		return m_data;
	}
	std::vector<AST::Expression*>& GetBases() { return m_bases; }
	virtual bool Call(XRuntime* rt, ARGS& params,
		KWARGS& kwParams,X::Value& retValue) override;
	inline void Add(X::LValue p)
	{
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
				m_bases.push_back(it);
			}
			m_bases.push_back(pThisBase);
		}
		else
		{//find common
			auto it = m_bases.begin();
			while (it != m_bases.end())
			{
				if (*it != pThisBase)
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
	inline void Add(Runtime* rt, X::Value& v)
	{
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
