#pragma once

#include "pycore.h"
#include "def.h"
#include <string>
#include <vector>
#include <stack>
#include <unordered_map>
#include "value.h"
#include "stackframe.h"

namespace XPython {namespace AST{
typedef AST::Value(*U_FUNC) (...);
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
	Func
};

class Scope;
class Var;
class Func;
class Expression
{
protected:
	Expression* m_parent = nil;
public:
	Expression()
	{
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
	virtual ~Expression()
	{
	}
	virtual void Set(Value& v)
	{

	}
	virtual bool Run(Value& v)
	{
		return false;
	}
	ObType m_type = ObType::Base;
};
class Operator :
	public Expression
{
protected:
	short Op;//index of _kws
	Alias A = Alias::None;
public:
	Operator()
	{
		Op = 0;
	}
	Operator(short op, Alias a)
	{
		Op = op;
		A = a;
	}
	inline short getOp()
	{
		return Op;
	}
	inline Alias getAlias() { return A; }
};
class Assign:
	public Operator
{
	Expression* L=nil;
	Expression* R =nil;
public:
	Assign(short op, Alias a):
		Operator(op,a)
	{
		m_type = ObType::Assign;
	}
	void SetLR(Expression* l, Expression* r)
	{
		L = l;
		R = r;
		if(L) L->SetParent(this);
		if(R) R->SetParent(this);
	}
	virtual bool Run(Value& v) override
	{
		if (!L || !R)
		{
			return false;
		}
		Value v_l;
		L->Run(v_l);
		Value v_r;
		if (R->Run(v_r))
		{
			L->Set(v_r);
		}
		return true;
	}
};

class BinaryOp :
	public Operator
{
	Expression* L=nil;
	Expression* R = nil;
public:
	BinaryOp(short op, Alias a):
		Operator(op,a)
	{
		m_type = ObType::BinaryOp;
	}
	void SetLR(Expression* l, Expression* r)
	{
		L = l;
		R = r;
		if(L) L->SetParent(this);
		if(R) R->SetParent(this);
	}
	virtual bool Run(Value& v) override
	{
		if (!L || !R)
		{
			return false;
		}
		Value v_l;
		if (!L->Run(v_l))
		{
			return false;
		}
		Value v_r;
		if (!R->Run(v_r))
		{
			return false;
		}
		bool bRet = true;
		switch (A)
		{
		case Alias::Add:
			v_l += v_r;
			v = v_l;
			break;
		case Alias::Minus:
			v_l -= v_r;
			v = v_l;
			break;
		case Alias::Multiply:
			v_l *= v_r;
			v = v_l;
			break;
		case Alias::Div:
			if (!v_r.IsZero())
			{
				v_l /= v_r;
				v = v_l;
			}
			else
			{
				bRet = false;
			}
			break;
		case Alias::Equal:
			v = Value(v_l == v_r);
			break;
		case Alias::NotEqual:
			v = Value(v_l != v_r);
			break;
		case Alias::Greater:
			v = Value(v_l > v_r);
			break;
		case Alias::Less:
			v = Value(v_l < v_r);
			break;
		case Alias::GreaterEqual:
			v = Value(v_l >= v_r);
			break;
		case Alias::LessEqual:
			v = Value(v_l <= v_r);
			break;
		case Alias::Dot:
		{
			int cnt = v_r.GetF();
			double d = (double)v_r.GetLongLong();
			for (int i = 0; i < cnt; i++)
			{
				d /= 10;
			}
			d += (double)v_l.GetLongLong();
			v = Value(d);
		}
		break;
		default:
			break;
		}
		return bRet;
	}
};
class PairOp :
	public Operator
{
	Expression* L = nil;
	Expression* R = nil;
public:
	PairOp(short opIndex,Alias a) :
		Operator(opIndex,a)
	{
		m_type = ObType::Pair;
	}
	void SetL(Expression* l)
	{
		L = l;
		if (L)
		{
			L->SetParent(this);
		}
	}
	void SetR(Expression* r)
	{
		R = r;
		if (R)
		{
			R->SetParent(this);
		}
	}
	Expression* GetR() { return R; }
	Expression* GetL() { return L; }
	virtual bool Run(Value& v) override;
};
class UnaryOp :
	public Operator
{
	Expression* R = nil;
public:
	UnaryOp(short op, Alias a):
		Operator(op,a)
	{
		m_type = ObType::UnaryOp;
	}
	void SetR(Expression* r)
	{
		R = r;
		if(R) R->SetParent(this);
	}
	virtual bool Run(Value& v) override;
};
class Range :
	public Operator
{
	Expression* R = nil;
	bool m_evaluated = false;
	long long m_start=0;
	long long m_stop =0;
	long long m_step = 1;

	bool Eval();
public:
	Range(short op, Alias a) :
		Operator(op, a)
	{
		m_type = ObType::Range;
	}
	void SetR(Expression* r)
	{
		R = r;
		if (R) R->SetParent(this);
	}
	virtual bool Run(Value& v) override;
};

class Var:
	public Expression
{
	String Name;

public:
	Var(String& n)
	{
		Name = n;
		m_type = ObType::Var;
	}
	String& GetName() { return Name; }
	virtual void Set(Value& v) override;
	virtual bool Run(Value& v) override;
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
	virtual bool Run(Value& v) override
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
	virtual bool Run(Value& v) override
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
	std::vector<Expression*>& GetList()
	{
		return list;
	}
	List(Expression* item):List()
	{
		list.push_back(item);
		if(item) item->SetParent(this);
	}
	List& operator+=(const List& rhs)
	{
		for (auto i : rhs.list)
		{
			list.push_back(i);
			if(i) i->SetParent(this);
		}
		return *this;
	}
	List& operator+=(Expression* item)
	{
		list.push_back(item);
		if(item) item->SetParent(this);
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
	inline Expression* GetName() { return Name; }
	inline Expression* GetType() { return Type; }
};
class Func;

struct Indent
{
	int tab_cnt =0;
	int space_cnt =0;
	bool operator>=(const Indent& other)
	{
		return (tab_cnt >= other.tab_cnt) 
			&& (space_cnt >= other.space_cnt);
	}
	bool operator==(const Indent& other)
	{
		return (tab_cnt == other.tab_cnt)
			&& (space_cnt == other.space_cnt);
	}
	bool operator<(const Indent& other)
	{
		return (tab_cnt <= other.tab_cnt && space_cnt < other.space_cnt)
			|| (tab_cnt < other.tab_cnt && space_cnt <= other.space_cnt);
	}
};
class Block :
	public Operator
{
	Indent IndentCount = { -1,-1 };
	Indent ChildIndentCount = { -1,-1 };
	std::vector<Expression*> Body;
public:
	Block() :Operator()
	{
	}
	void Add(Expression* item)
	{
		Body.push_back(item);
		if(item) item->SetParent(this);
	}
	virtual Func* FindFuncByName(Var* name);
	inline Indent GetIndentCount() { return IndentCount; }
	inline Indent GetChildIndentCount() { return ChildIndentCount; }
	inline void SetIndentCount(Indent cnt) { IndentCount = cnt; }
	inline void SetChildIndentCount(Indent cnt) { ChildIndentCount = cnt; }
	virtual bool Run(Value& v) override
	{
		bool bOk = true;
		for (auto i : Body)
		{
			Value v0;
			bOk = i->Run(v0);
			if (!bOk)
			{
				break;
			}
		}
		return bOk;
	}
};
class InOp :
	public Operator
{
	Expression* m_var = nil;
	Expression* m_exp = nil;
public:
	InOp(short op, Alias a) :
		Operator(op, a)
	{
	}
	void Set(Expression* var, Expression* e)
	{
		m_var = var;
		m_exp = e;
		if (m_var) m_var->SetParent(this);
		if (m_exp) m_exp->SetParent(this);
	}
	virtual bool Run(Value& v) override;
};
class For :
	public Block
{
	Expression* m_condition = nil;
public:
	For(short op, Alias a)
	{
		Op = op;
		A = a;
	}
	void SetCondition(Expression* e)
	{
		m_condition = e;
		if (m_condition)
		{
			m_condition->SetParent(this);
		}
	}
	virtual bool Run(Value& v) override;
};
class Scope:
	public Block
{//variables scope support, for Module and Func/Class
protected:
	std::stack<StackFrame*> mStackFrames;
public:
	Scope() :
		Block()
	{
	}
	void PushFrame(StackFrame* frame)
	{
		mStackFrames.push(frame);
	}
	StackFrame* PopFrame()
	{
		return mStackFrames.empty() ? nil : mStackFrames.top();
	}
	bool Have(std::string& name)
	{
		return mStackFrames.empty()?false:
			mStackFrames.top()->Have(name);
	}
	bool Set(std::string& name, Value& v)
	{
		return mStackFrames.empty() ? false :
			mStackFrames.top()->Set(name,v);
	}
	bool SetReturn(Value& v)
	{
		return mStackFrames.empty() ? false :
			mStackFrames.top()->SetReturn(v);
	}
	bool Get(std::string& name, Value& v)
	{
		return mStackFrames.empty() ? false :
			mStackFrames.top()->Get(name, v);
	}
};
class Module :
	public Scope
{
public:
	Module() :
		Scope()
	{
		SetIndentCount({ -1,-1 });//then each line will have 0 indent
	}
};
class Func :
	public Scope
{
	Expression* Name = nil;
	List* Params =nil;
	Expression* RetType = nil;
	bool SetParamsIntoFrame(StackFrame* frame, List* param_values);
public:
	Func():
		Scope()
	{
		m_type = ObType::Func;
	}
	Expression* GetName() { return Name; }
	void SetName(Expression* n)
	{
		Name = n;
		if (Name)
		{
			Name->SetParent(this);
		}
	}
	void SetParams(List* p)
	{
		Params = p;
		if (Params)
		{
			Params->SetParent(this);
		}
	}
	void SetRetType(Expression* p)
	{
		RetType = p;
		if (RetType)
		{
			RetType->SetParent(this);
		}
	}
	virtual bool Call(List* params,Value& retValue);
	virtual bool Run(Value& v) override
	{// func doesn't need to run in module
	 // but will call by callee
		return true;
	}
};
class ExternFunc
	:public Func
{
	std::string m_funcName;
	U_FUNC m_func;
public:
	ExternFunc(std::string& funcName,U_FUNC func)
	{
		m_funcName = funcName;
		m_func = func;
	}
	virtual bool Call(List* params, Value& retValue) override;
};
}}