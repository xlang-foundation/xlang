#pragma once

#include <string>

namespace XPython {namespace AST {
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
	ValueType GetType() { return t; }
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
	Value(char* s, int size)
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
			if (rhs.x.l != 0)
			{
				x.l /= rhs.x.l;
			}
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
	friend Value& operator+(Value lhs, const Value& rhs)
	{
		lhs += rhs;
		return lhs;
	}
	std::string ToString();
};
}
}
