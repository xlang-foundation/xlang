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
	PipeOp,
	Range,
	Var,
	Number,
	Double,
	Param,
	List,
	Pair,
	Dot,
	Func,
	BuiltinFunc,
	Module,
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
	void SetParent(Expression* p)
	{
		m_parent = p;
	}
	Expression* GetParent()
	{
		return m_parent;
	}
	virtual bool CalcCallables(Runtime* rt, void* pContext,
		std::vector<Expression*>& callables)
	{
		return false;
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
	bool m_haveFormat = false;
	char* m_s = nil;
	int m_size = 0;
public:
	Str(char* s, int size,bool haveFormat)
	{
		m_haveFormat = haveFormat;
		m_s = s;
		m_size = size;
	}
	bool RunWithFormat(Runtime* rt, void* pContext, Value& v, LValue* lValue);
	virtual bool Run(Runtime* rt,void* pContext, Value& v,LValue* lValue=nullptr) override
	{
		if (m_haveFormat)
		{
			return RunWithFormat(rt, pContext, v, lValue);
		}
		else
		{
			v = Value(m_s, m_size);
			return true;
		}
	}
};
class Number :
	public Expression
{
	long long m_val;
	int m_digiNum = 0;
	bool m_isBool = false;
public:
	Number(long long val, int num=0)
	{
		m_val = val;
		m_digiNum = num;
		m_type = ObType::Number;
	}
	Number(bool val)
	{
		m_val = val?1:0;
		m_type = ObType::Number;
		m_isBool = true;
	}
	inline long long GetVal() { return m_val; }
	inline int GetDigiNum() { return m_digiNum; }
	virtual bool Run(Runtime* rt,void* pContext, Value& v,LValue* lValue=nullptr) override
	{
		Value v0(m_val);
		if (m_isBool)
		{
			v0.AsBool();
		}
		else
		{
			v0.SetF(m_digiNum);
		}
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
	virtual bool CalcCallables(Runtime* rt, void* pContext,
		std::vector<Expression*>& callables) override
	{
		bool bHave = false;
		for (auto it : list)
		{
			bHave |= it->CalcCallables(rt, pContext, callables);
		}
		return bHave;
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
	virtual bool CalcCallables(Runtime* rt, void* pContext,
		std::vector<Expression*>& callables) override
	{
		bool bHave = Name ? Name->CalcCallables(rt, pContext, callables) : false;
		bHave |= Type ? Type->CalcCallables(rt, pContext, callables) : false;
		return bHave;
	}
	virtual void ScopeLayout() override
	{
		if (Name) Name->ScopeLayout();
		if (Type) Type->ScopeLayout();
	}
};
}
}