#pragma once
#include "object.h"
namespace X
{
namespace Data
{

class List :
	public Object
{
protected:
	bool m_useLValue = false;
	std::vector<AST::Value> m_data;
	std::vector<AST::LValue> m_ptrs;
	std::vector<AST::Expression*> m_bases;
public:
	List() :
		Object()
	{
		m_t = Type::List;

	}
	~List()
	{
		m_bases.clear();
		m_ptrs.clear();
		m_data.clear();
	}
	virtual std::string ToString() override
	{
		std::string strList = "[\n";
		size_t size = Size();
		for (size_t i = 0; i < size; i++)
		{
			AST::Value v0;
			Get(i, v0);
			strList += '\t' + v0.ToString() + ",\n";
		}
		strList += "]";
		return strList;
	}
	inline size_t Size() { return m_useLValue ? m_ptrs.size() : m_data.size(); }
	std::vector<AST::Value>& Data()
	{
		return m_data;
	}
	std::vector<AST::Expression*>& GetBases() { return m_bases; }
	virtual bool Call(Runtime* rt, std::vector<AST::Value>& params,
		std::unordered_map<std::string, AST::Value>& kwParams,
		AST::Value& retValue)
	{
		//do twice, first to do size or other call with
		//memory allocation
		for (auto it : kwParams)
		{
			if (it.first == "size")
			{
				long long size = it.second.GetLongLong();
				m_data.resize(size);
			}
		}
		for (auto it : kwParams)
		{
			if (it.first == "init")
			{
				for (auto& v : m_data)
				{
					v = it.second;
				}
			}
		}
		return true;
	}
	inline void Add(AST::LValue p)
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
	inline void Add(Runtime* rt, AST::Value& v)
	{
		if (v.IsObject())
		{
			Object* obj = (Object*)v.GetObj();
			if (obj->GetType() == Data::Type::XClassObject)
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
			else if (obj->GetType() == Data::Type::Function)
			{
				std::vector<AST::XClass*> dummy;
				MakeCommonBases(rt->M(), dummy);
			}
		}
		m_data.push_back(v);
	}
	inline bool Get(long long idx, AST::Value& v,
		AST::LValue* lValue = nullptr)
	{
		if (m_useLValue)
		{
			if (idx >= (long long)m_ptrs.size())
			{
				return false;
			}
			AST::LValue l = m_ptrs[idx];
			v = *l;
			if (lValue) *lValue = l;
		}
		else
		{
			if (idx >= (long long)m_data.size())
			{
				m_data.resize(idx + 1);
			}
			AST::Value& v0 = m_data[idx];
			v = v0;
			if (lValue) *lValue = &v0;
		}
		return true;
	}
};
}
}
