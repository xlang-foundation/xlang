#pragma once

#include "pycore.h"
#include "def.h"
#include <string>
#include <vector>
#include <unordered_map>

namespace XPython {namespace AST{
enum class ValueType
{
	None,
	Int64,
	Double,
	Pointer,
	Str,

};
class Value
{
	int flags = 0;
	ValueType t;
	union 
	{
		long long l;
		double d;
		void* p;
	} x;
public:
	double GetDouble()
	{
		return x.d;
	}
	long long GetLongLong()
	{
		return x.l;
	}
	void SetF(int f)
	{
		flags = f;
	}
	int GetF() { return flags; }
	Value()
	{
		t = ValueType::None;
		x.l = 0;
	}
	Value(long long l)
	{
		t = ValueType::Int64;
		x.l = l;
	}
	Value(double d)
	{
		t = ValueType::Double;
		x.d = d;
	}
	Value(char* s,int size)
	{
		t = ValueType::Str;
		flags = size;
		x.p = s;
	}
	Value(void* p)
	{
		t = ValueType::Pointer;
		x.p = p;
	}
	Value(Value& v)
	{
		flags = v.flags;
		t = v.t;
		switch (t)
		{
		case ValueType::None:
			break;
		case ValueType::Int64:
			x.l = v.x.l;
			break;
		case ValueType::Double:
			x.d = v.x.d;
			break;
		case ValueType::Pointer:
			x.p = v.x.p;
			break;
		default:
			break;
		}
	}
	Value& operator+=(const Value& rhs)
	{
		switch (t)
		{
		case ValueType::None:
			break;
		case ValueType::Int64:
			x.l += rhs.x.l;
			break;
		case ValueType::Double:
			if (rhs.t == ValueType::Int64)
			{
				x.d += rhs.x.l;
			}
			else
			{
				x.d += rhs.x.d;
			}
			break;
		case ValueType::Pointer:
			break;
		default:
			break;
		}
		return *this;
	}
	Value& operator*=(const Value& rhs)
	{
		switch (t)
		{
		case ValueType::None:
			break;
		case ValueType::Int64:
			if (rhs.t == ValueType::Int64)
			{
				x.l *= rhs.x.l;
			}
			else
			{
				x.l *= (long long)rhs.x.d;
			}
			break;
		case ValueType::Double:
			if (rhs.t == ValueType::Int64)
			{
				x.d *= rhs.x.l;
			}
			else
			{
				x.d *= rhs.x.d;
			}
			break;
		case ValueType::Pointer:
			break;
		default:
			break;
		}
		return *this;
	}
	Value& operator-=(const Value& rhs)
	{
		switch (t)
		{
		case ValueType::None:
			break;
		case ValueType::Int64:
			x.l -= rhs.x.l;
			break;
		case ValueType::Double:
			x.d -= rhs.x.d;
			break;
		case ValueType::Pointer:
			break;
		default:
			break;
		}
		return *this;
	}
	Value& operator/=(const Value& rhs)
	{
		switch (t)
		{
		case ValueType::None:
			break;
		case ValueType::Int64:
			x.l /= rhs.x.l;
			break;
		case ValueType::Double:
			x.d /= rhs.x.d;
			break;
		case ValueType::Pointer:
			break;
		default:
			break;
		}
		return *this;
	}
	friend Value& operator+(Value lhs,const Value& rhs)
	{
		lhs += rhs;
		return lhs;
	}
};

enum class ObType
{
	Base,
	Assign,
	BinaryOp,
	UnaryOp,
	Var,
	Number,
	Double,
	Param,
	List,
	Pair,
	Func
};

class Scope;
class Expression
{
protected:
	Expression* m_parent = nil;
public:
	Expression()
	{
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
	Assign(short op)
	{
		Op = op;
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
	BinaryOp(short op)
	{
		Op = op;
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
		return true;
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
		A = a;
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
	UnaryOp(short op)
	{
		Op = op;
		m_type = ObType::UnaryOp;
	}
	void SetR(Expression* r)
	{
		R = r;
		if(R) R->SetParent(this);
	}
	virtual bool Run(Value& v) override
	{
		Value v_r;
		if (!R->Run(v_r))
		{
			return false;
		}
		switch (A)
		{
		case Alias::Add:
			//+ keep 
			break;
		case Alias::Minus:
			v = Value((long long)0);//set to 0
			v-= v_r;
			break;
		default:
			break;
		}
		return true;
	}
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
};
class Func;
class Block :
	public Operator
{
	int IndentCount = 0;
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
	inline int GetIndentCount() { return IndentCount; }
	inline void SetIndentCount(int cnt) { IndentCount = cnt; }
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
class Scope:
	public Block
{//variables scope support, for Module and Func/Class
protected:
	std::unordered_map < std::string, Value> _VarMap;
public:
	Scope() :
		Block()
	{
	}
	bool Have(std::string& name)
	{
		auto it = _VarMap.find(name);
		return (it != _VarMap.end());
	}
	bool Set(std::string& name, Value& v)
	{
		_VarMap[name] = v;
		return true;
	}
	bool Get(std::string& name, Value& v)
	{
		bool bFind = false;
		auto it = _VarMap.find(name);
		if (it != _VarMap.end())
		{
			v = it->second;
			bFind = true;
		}
		return bFind;
	}
};
class Module :
	public Scope
{
public:
	Module() :
		Scope()
	{
		SetIndentCount(-1);//then each line will have 0 indent
	}
};
class Func :
	public Scope
{
	Expression* Name = nil;
	List* Params =nil;
	Expression* RetType = nil;
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
	bool Call(List* params,Value& retValue);
	virtual bool Run(Value& v) override
	{// func doesn't need to run in module
	 // but will call by callee
		return true;
	}
};
}}