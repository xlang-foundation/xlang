#pragma once

#include <string>

namespace X {namespace AST {
enum class ValueType
{
	None,
	Int64,
	Double,
	Object,
	Str,
};

#define ToDouble(v) \
	((v.t == ValueType::Int64) ? (double)v.x.l:((v.t == ValueType::Double)?v.x.d:0.0))
#define ToInt64(v) \
	((v.t == ValueType::Int64) ? v.x.l:((v.t == ValueType::Double)?(long long)v.x.d:0))


#define ARITH_OP(op)\
Value& operator op (const Value& r)\
{\
	switch (t)\
	{\
	case ValueType::None:\
		break;\
	case ValueType::Int64:\
		x.l op ToInt64(r);\
		break;\
	case ValueType::Double:\
		x.d op ToDouble(r);\
		break;\
	case ValueType::Object:\
		break;\
	default:\
		break;\
	}\
	return *this;\
}


#define COMPARE_OP(op)\
bool operator op (const Value& r)\
{\
	bool bRet = false;\
	switch (t)\
	{\
	case ValueType::None:\
		break;\
	case ValueType::Int64:\
		bRet = (x.l op ToInt64(r));\
		break;\
	case ValueType::Double:\
		bRet = (x.d op ToDouble(r));\
		break;\
	case ValueType::Object:\
		break;\
	default:\
		break;\
	}\
	return bRet;\
}

#define BOOL_FLAG -10
class Value
{
	int flags = 0;
	ValueType t;
	union
	{
		long long l;
		double d;
		void* p;
	}x;
public:
	double GetDouble()
	{
		return x.d;
	}
	long long GetLongLong()
	{
		return x.l;
	}
	void* GetObject()
	{
		return x.p;
	}
	void SetF(int f)
	{
		flags = f;
	}
	inline bool IsObject()
	{
		return (t == ValueType::Object);
	}
	inline bool IsTrue()
	{
		return !IsZero();
	}
	inline bool IsZero()
	{
		bool bRet = false;
		switch (t)
		{
		case ValueType::None:
			bRet = true;
			break;
		case ValueType::Int64:
			bRet = (x.l == 0);
			break;
		case ValueType::Double:
			bRet = (x.d == 0);
			break;
		case ValueType::Object:
			bRet = (x.p == 0);
			break;
		default:
			break;
		}
		return bRet;
	}
	ValueType GetType() { return t; }
	int GetF() { return flags; }
	Value()
	{
		t = ValueType::None;
		x.l = 0;
	}
	Value(bool b)
	{//use 1 as true and 0 as false, set flag to -1
		t = ValueType::Int64;
		flags = BOOL_FLAG;
		x.l = b ? 1 : 0;
	}
	Value(int l)
	{
		t = ValueType::Int64;
		x.l = l;
	}
	Value(long l)
	{
		t = ValueType::Int64;
		x.l = l;
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
		t = ValueType::Object;
		x.p = p;
	}
	Value(const Value& v)
	{
		flags = v.flags;
		t = v.t;
		switch (t)
		{
		case ValueType::Int64:
			x.l = ToInt64(v);
			break;
		case ValueType::Double:
			x.d = ToDouble(v);
			break;
		case ValueType::Str:
			x.p = v.x.p;
			break;
		case ValueType::Object:
			x.p = v.x.p;
			break;
		default:
			break;
		}
	}
	void operator = (const Value& v)
	{
		flags = v.flags;
		t = v.t;
		switch (t)
		{
		case ValueType::Int64:
			x.l = ToInt64(v);
			break;
		case ValueType::Double:
			x.d = ToDouble(v);
			break;
		case ValueType::Str:
			x.p = v.x.p;
			break;
		case ValueType::Object:
			x.p = v.x.p;
			break;
		default:
			break;
		}
	}
	ARITH_OP(+= );
	ARITH_OP(-= );
	ARITH_OP(*= );
	ARITH_OP(/= );
	COMPARE_OP(==);
	COMPARE_OP(!=);
	COMPARE_OP(>);
	COMPARE_OP(<);
	COMPARE_OP(>=);
	COMPARE_OP(<=);

	std::string ToString();
};
typedef Value* LValue;
}
}
