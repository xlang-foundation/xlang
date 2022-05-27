#pragma once

#include "pycore.h"

namespace XPython {namespace AST{


enum class ValueType
{
	None,
	Int64,
	Double,
	Pointer
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
};

class Expression
{
public:
	Expression()
	{
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
public:
	inline short getOp()
	{
		return Op;
	}
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
		KWIndex kwOp = (KWIndex)Op;
		switch (kwOp)
		{
		case KWIndex::Add:
			v_l += v_r;
			v = v_l;
			break;
		case KWIndex::Dot:
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
	}
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
	}
	virtual bool Run(Value& v) override
	{
		Value v_r;
		if (!R->Run(v_r))
		{
			return false;
		}
		KWIndex kwOp = (KWIndex)Op;
		switch (kwOp)
		{
		case KWIndex::Add:
			//+ keep 
			break;
		case KWIndex::Minus:
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
	virtual void Set(Value& v) override;
	virtual bool Run(Value& v) override;
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

}}