#pragma once
#include <vector>
#include <string>
#include "value.h"
#include "exp.h"

namespace X {namespace Data{
enum class Type
{
	Base,
	Expr,
	Function,
	XClassObject,
	FuncCalls,
	List,
	Dict
};
class Object
{
protected:
	int m_ref = 0;
	Type m_t = Type::Base;
public:
	Object()
	{
	}
	Type GetType() { return m_t; }
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
		m_t = Type::Expr;
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
		m_t = Type::Function;
		m_func = p;
	}
	AST::Func* GetFunc() { return m_func; }
	virtual bool Call(std::vector<AST::Value>& params,
		std::unordered_map<std::string, AST::Value>& kwParams,
		AST::Value& retValue)
	{
		return m_func->Call(nullptr,params, retValue);
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
		m_t = Type::XClassObject;
		m_obj = p;
	}
	AST::XClass* GetClassObj() { return m_obj; }
	virtual bool Call(std::vector<AST::Value>& params,
		std::unordered_map<std::string, AST::Value>& kwParams,
		AST::Value& retValue)
	{
		return m_obj->Call(params, retValue);
	}
};
struct FuncCall
{
	XClassObject* m_thisObj = nil;
	AST::Func* m_func = nil;
};
class List :
	public Object
{
protected:
	std::vector<AST::Value> m_data;
	std::vector<AST::Expression*> m_bases;
public:
	List():
		Object()
	{
		m_t = Type::List;

	}
	std::vector<AST::Value>& Data()
	{
		return m_data;
	}
	std::vector<AST::Expression*>& GetBases() { return m_bases; }
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
		if (v.IsObject())
		{
			Object* obj = (Object*)v.GetObject();
			XClassObject* pClassObj = dynamic_cast<XClassObject*>(obj);
			if (pClassObj)
			{
				AST::XClass* pXClass = pClassObj->GetClassObj();
				if (pXClass)
				{
					auto& bases_0 = pXClass->GetBases();
					if (m_bases.empty())//first item
					{//append all
						for (auto it : bases_0)
						{
							m_bases.push_back(it);
						}
						m_bases.push_back(pXClass);
					}
					else
					{//find common
						auto it = m_bases.begin();
						while (it != m_bases.end())
						{
							if (*it != pXClass)
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
			}
		}
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
		m_t = Type::Dict;
	}
	virtual bool Call(std::vector<AST::Value>& params,
		std::unordered_map<std::string, AST::Value>& kwParams,
		AST::Value& retValue)
	{
		return true;
	}
};
class FuncCalls :
	public Object
{
protected:
	std::vector<FuncCall> m_list;
public:
	FuncCalls()
	{
		m_t = Type::Function;
	}
	void Add(XClassObject* thisObj, AST::Func* func)
	{
		m_list.push_back(FuncCall{ thisObj ,func });
	}
	virtual bool Call(std::vector<AST::Value>& params,
		std::unordered_map<std::string, AST::Value>& kwParams,
		AST::Value& retValue)
	{
		if (m_list.size() == 1)
		{
			auto& fc = m_list[0];
			return fc.m_func->Call(fc.m_thisObj,params, retValue);
		}
		List* pValueList = new List();
		bool bOK = true;
		for (auto& fc : m_list)
		{
			AST::Value v0;
			bool bOK = fc.m_func->Call(fc.m_thisObj,params, v0);
			if (bOK)
			{
				pValueList->Add(v0);
			}
			else
			{
				break;
			}
		}
		if (bOK)
		{
			retValue = AST::Value(pValueList);
		}
		else
		{
			delete pValueList;
		}
		return bOK;
	}
};

}
}

