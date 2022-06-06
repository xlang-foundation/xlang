#pragma once
#include <vector>
#include "value.h"

namespace X {namespace Data{
class Object
{
protected:
	int m_ref = 0;
public:
	Object()
	{
	}
};
class List :
	public Object
{
protected:
	std::vector<AST::Value> m_data;
public:
	List():
		Object()
	{

	}
	inline void Add(AST::Value& v)
	{
		m_data.push_back(v);
	}
	inline bool Get(long long idx, AST::Value& v,
		AST::LValue* lValue=nullptr)
	{
		if (idx >= m_data.size())
		{
			m_data.resize(idx + 1);
		}
		AST::Value& v0 = m_data[idx];
		v = v0;
		if (lValue) *lValue = &v0;
		return true;
	}
};

}
}

