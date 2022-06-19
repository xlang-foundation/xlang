#pragma once
#include "def.h"
#include <string>
#include <vector>
#include "value.h"
#include "glob.h"

namespace X 
{
class Runtime;
namespace AST
{
enum class ObType
{
	Base,
	Assign,
	BinaryOp,
	UnaryOp,
	Range,
	Var,
	Number,
	Double,
	Param,
	List,
	Pair,
	Dot,
	Func,
	Class,
	Import
};
class Func;
class Scope;
class Var;
class Expression
{
protected:
	Expression* m_parent = nil;
	Scope* m_scope = nil;//set by compiling
	bool m_isLeftValue = false;
	//hint
	int m_lineStart=0;
	int m_lineEnd=0;
	int m_charPos=0;
public:
	Expression()
	{
	}
	inline void SetHint(int startLine, int endLine, int charPos)
	{
		m_lineStart = startLine;
		m_lineEnd = endLine;
		m_charPos = charPos;
	}
	inline int GetStartLine() { return m_lineStart+1; }
	inline int GetEndLine() { return m_lineEnd+1; }
	inline int GetCharPos() { return m_charPos; }
	inline void SetIsLeftValue(bool b)
	{
		m_isLeftValue = b;
	}
	inline bool IsLeftValue() { return m_isLeftValue; }
	virtual ~Expression(){}
	inline Scope* GetScope()
	{
		if (m_scope == nil)
		{
			m_scope = FindScope();//FindScope will AddRef
		}
		return m_scope;
	}
	Scope* FindScope();
	Func* FindFuncByName(Var* name);
	void SetParent(Expression* p)
	{
		m_parent = p;
	}
	Expression* GetParent()
	{
		return m_parent;
	}
	virtual void Set(Runtime* rt,void* pContext, Value& v){}
	virtual bool Run(Runtime* rt,void* pContext,Value& v,LValue* lValue=nullptr)
	{
		return false;
	}
	virtual bool EatMe(Expression* other)
	{
		return false;
	}
	virtual void ScopeLayout() {}
	virtual int GetLeftMostCharPos() { return m_charPos; }

	ObType m_type = ObType::Base;
};
class Str :
	public Expression
{
	char* m_s = nil;
	int m_size = 0;
public:
	Str(char* s, int size)
	{
		m_s = s;
		m_size = size;
	}
	virtual bool Run(Runtime* rt,void* pContext, Value& v,LValue* lValue=nullptr) override
	{
		Value v0(m_s,m_size);
		v = v0;
		return true;
	}
};
class Number :
	public Expression
{
	long long m_val;
	int m_digiNum = 0;
public:
	Number(long long val, int num=0)
	{
		m_val = val;
		m_digiNum = num;
		m_type = ObType::Number;
	}
	virtual bool Run(Runtime* rt,void* pContext, Value& v,LValue* lValue=nullptr) override
	{
		Value v0(m_val);
		v0.SetF(m_digiNum);
		v = v0;
		return true;
	}
};
class Double :
	public Expression
{
	double m_val;
public:
	Double(double val)
	{
		m_val = val;
		m_type = ObType::Double;
	}
};
class List :
	public Expression
{
	std::vector<Expression*> list;
public:
	List()
	{
		m_type = ObType::List;
	}
	void ClearList() { list.clear();}//before this call,
	//copy all list into another List
	~List()
	{
		for (auto e : list)
		{
			delete e;
		}
	}
	virtual int GetLeftMostCharPos() override
	{
		if (list.size() > 0)
		{
			return list[0]->GetLeftMostCharPos();
		}
		else
		{
			return 9999;
		}
	}
	virtual void ScopeLayout() override
	{
		for (auto i : list)
		{
			i->ScopeLayout();
		}
	}
	std::vector<Expression*>& GetList()
	{
		return list;
	}
	List(Expression* item):List()
	{
		list.push_back(item);
		if (item)
		{
			item->SetParent(this);
		}
	}
	List& operator+=(const List& rhs)
	{
		for (auto i : rhs.list)
		{
			list.push_back(i);
			if (i)
			{
				i->SetParent(this);
			}
		}
		return *this;
	}
	List& operator+=(Expression* item)
	{
		list.push_back(item);
		if (item)
		{
			item->SetParent(this);
		}
		return *this;
	}
};
class Param :
	public Expression
{
	Expression* Name = nil;
	Expression* Type = nil;
public:
	Param(Expression* name, Expression* type)
	{
		Name = name;
		Type = type;
		if (Name)
		{
			Name->SetParent(this);
		}
		if (Type)
		{
			Type->SetParent(this);
		}
		m_type = ObType::Param;
	}
	~Param()
	{
		if (Name) delete Name;
		if (Type) delete Type;
	}
	inline Expression* GetName() { return Name; }
	inline Expression* GetType() { return Type; }
	bool Parse(std::string& strVarName,
		std::string& strVarType,
		Value& defaultValue);
};
}
}