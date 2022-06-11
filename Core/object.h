#pragma once
#include <vector>
#include <string>
#include "value.h"
#include "exp.h"

namespace X {namespace Data{
class Object
{
protected:
	int m_ref = 0;
public:
	Object()
	{
	}
	virtual bool Call(std::vector<AST::Value>& params,
		std::unordered_map<std::string, AST::Value>& kwParams,
		AST::Value& retValue) = 0;
};
class Expr
	:public Object
{//any valid AST tree with one root
protected:
	AST::Expression* m_expr = nullptr;
public:
	Expr(AST::Expression* e)
	{
		m_expr = e;
	}
	virtual bool Call(std::vector<AST::Value>& params,
		std::unordered_map<std::string, AST::Value>& kwParams,
		AST::Value& retValue)
	{
		return true;
	}
};
class Function :
	public Object
{
protected:
	AST::Func* m_func = nullptr;
public:
	Function(AST::Func* p)
	{
		m_func = p;
	}
	virtual bool Call(std::vector<AST::Value>& params,
		std::unordered_map<std::string, AST::Value>& kwParams,
		AST::Value& retValue)
	{
		return m_func->Call(params, retValue);
	}
};
class XClassObject :
	public Object
{
protected:
	AST::XClass* m_obj = nullptr;
public:
	XClassObject(AST::XClass* p)
	{
		m_obj = p;
	}
	virtual bool Call(std::vector<AST::Value>& params,
		std::unordered_map<std::string, AST::Value>& kwParams,
		AST::Value& retValue)
	{
		return m_obj->Call(params, retValue);
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
	virtual bool Call(std::vector<AST::Value>&params,
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
	inline void Add(AST::Value& v)
	{
		m_data.push_back(v);
	}
	inline bool Get(long long idx, AST::Value& v,
		AST::LValue* lValue=nullptr)
	{
		if (idx >= (long long)m_data.size())
		{
			m_data.resize(idx + 1);
		}
		AST::Value& v0 = m_data[idx];
		v = v0;
		if (lValue) *lValue = &v0;
		return true;
	}
};
class Dict :
	public Object
{
protected:
	std::vector<Object*> m_bases;
	std::vector<std::string> m_keys;
public:
	Dict()
	{

	}
	virtual bool Call(std::vector<AST::Value>& params,
		std::unordered_map<std::string, AST::Value>& kwParams,
		AST::Value& retValue)
	{
		return true;
	}
};
}
}

