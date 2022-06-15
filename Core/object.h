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
	virtual bool Call(AST::Module* pModule,std::vector<AST::Value>& params,
		std::unordered_map<std::string, AST::Value>& kwParams,
		AST::Value& retValue) = 0;
	virtual std::string ToString()
	{
		char v[1000];
		snprintf(v, sizeof(v), "Object:0x%llx", 
			(unsigned long long)this);
		return v;
	}
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
	AST::Expression* Get() { return m_expr; }
	virtual bool Call(AST::Module* pModule, std::vector<AST::Value>& params,
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
	virtual bool Call(AST::Module* pModule, std::vector<AST::Value>& params,
		std::unordered_map<std::string, AST::Value>& kwParams,
		AST::Value& retValue)
	{
		return m_func->Call(pModule,nullptr,params, retValue);
	}
};
class XClassObject :
	public Object
{
protected:
	AST::XClass* m_obj = nullptr;
	AST::StackFrame* m_stackFrame = nullptr;
public:
	XClassObject(AST::XClass* p)
	{
		m_t = Type::XClassObject;
		m_obj = p;
		m_stackFrame = new AST::StackFrame();
		m_stackFrame->SetVarCount(p->GetVarNum());
		auto* pClassStack = p->GetClassStack();
		if (pClassStack)
		{
			m_stackFrame->Copy(pClassStack);
		}
	}
	inline AST::StackFrame* GetStack()
	{
		return m_stackFrame;
	}
	AST::XClass* GetClassObj() { return m_obj; }
	virtual bool Call(AST::Module* pModule, std::vector<AST::Value>& params,
		std::unordered_map<std::string, AST::Value>& kwParams,
		AST::Value& retValue)
	{
		return m_obj->Call(pModule,params, retValue);
	}
};

class List :
	public Object
{
protected:
	bool m_useLValue = false;
	std::vector<AST::Value> m_data;
	std::vector<AST::LValue> m_ptrs;
	std::vector<AST::Expression*> m_bases;
public:
	List():
		Object()
	{
		m_t = Type::List;

	}
	virtual std::string ToString() override
	{
		std::string strList = "[\n";
		size_t size = Size();
		for (size_t i = 0; i < size; i++)
		{
			AST::Value v0;
			Get(i, v0);
			strList += '\t'+v0.ToString()+",\n";
		}
		strList += "]";
		return strList;
	}
	inline size_t Size() { return m_useLValue? m_ptrs.size():m_data.size(); }
	std::vector<AST::Value>& Data()
	{
		return m_data;
	}
	std::vector<AST::Expression*>& GetBases() { return m_bases; }
	virtual bool Call(AST::Module* pModule, std::vector<AST::Value>&params,
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
	virtual bool Call(void* pLineExpr, std::vector<AST::Value>& params,
		std::unordered_map<std::string, AST::Value>& kwParams,
		AST::Value& retValue)
	{
		return true;
	}
};

struct VectorCall
{
	XClassObject* m_thisObj = nil;
	AST::Func* m_func = nil;
	AST::LValue m_lVal = nil;
};
class FuncCalls :
	public Object
{
protected:
	std::vector<VectorCall> m_list;
public:
	FuncCalls()
	{
		m_t = Type::FuncCalls;
	}
	void Add(XClassObject* thisObj, AST::Func* func, AST::LValue lVal)
	{
		m_list.push_back(VectorCall{ thisObj ,func,lVal });
	}
	bool SetValue(AST::Value& val)
	{
		for (auto& i : m_list)
		{
			if (i.m_lVal)
			{
				*i.m_lVal = val;
			}
		}
		return true;
	}
	virtual bool Call(AST::Module* pModule, std::vector<AST::Value>& params,
		std::unordered_map<std::string, AST::Value>& kwParams,
		AST::Value& retValue)
	{
		if (m_list.size() == 1)
		{
			auto& fc = m_list[0];
			return fc.m_func->Call(pModule,
				fc.m_thisObj,params, retValue);
		}
		List* pValueList = new List();
		bool bOK = true;
		for (auto& fc : m_list)
		{
			AST::Value v0;
			bool bOK = fc.m_func->Call(pModule,
				fc.m_thisObj,params, v0);
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

