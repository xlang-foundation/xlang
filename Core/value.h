#pragma once

#include <string>

namespace X {
	namespace Data { class Object; }
namespace AST {
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
#define ToStr(p,size) \
	std::string((char*)p,(size_t)size)

#define ARITH_OP(op)\
Value& operator op (const Value& r);

#define ARITH_OP_IMPL(op)\
Value& Value::operator op (const Value& r)\
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
	case ValueType::Str:\
		ChangeToStrObject();\
	case ValueType::Object:\
		{\
			Value v =r;\
			(*((Data::Object*)x.obj)) op v;\
		}\
		break;\
	default:\
		break;\
	}\
	return *this;\
}


#define COMPARE_OP(op)\
bool operator op (const Value& r) const\
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
	case ValueType::Str:\
		bRet = (ToStr(x.str,flags) op ToStr(r.x.str,r.flags));\
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
		char* str;
		Data::Object* obj;
	}x;
public:
	inline ~Value()
	{
		switch (t)
		{
		case ValueType::Str:
			break;
		case ValueType::Object:
			ReleaseObject(x.obj);
			break;
		default:
			break;
		}

	}
	inline Value()
	{
		t = ValueType::None;
		x.l = 0;
	}
	inline Value(bool b)
	{//use 1 as true and 0 as false, set flag to -1
		t = ValueType::Int64;
		flags = BOOL_FLAG;
		x.l = b ? 1 : 0;
	}
	inline Value(int l)
	{
		t = ValueType::Int64;
		x.l = l;
	}
	inline Value(long l)
	{
		t = ValueType::Int64;
		x.l = l;
	}
	inline Value(long long l)
	{
		t = ValueType::Int64;
		x.l = l;
	}
	inline Value(double d)
	{
		t = ValueType::Double;
		x.d = d;
	}
	inline Value(char* s, int size)
	{
		t = ValueType::Str;
		flags = size;
		x.str = s;
	}
	inline Value(Data::Object* p)
	{
		t = ValueType::Object;
		AssignObject(p);
	}
	bool ChangeToStrObject();
	void AssignObject(Data::Object* p);
	void ReleaseObject(Data::Object* p);
	inline Value(const Value& v)
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
			x.str = v.x.str;
			break;
		case ValueType::Object:
			AssignObject(v.x.obj);
			break;
		default:
			break;
		}
	}
	inline size_t Hash()
	{
		size_t h = 0;
		switch (t)
		{
		case ValueType::Int64:
			h = std::hash<long long>{}(x.l);
			break;
		case ValueType::Double:
			h = std::hash<double>{}(x.d);
			break;
		case ValueType::Str:
			h = std::hash<std::string>{}(std::string((char*)x.str, 
				(size_t)flags));
			break;
		case ValueType::Object:
			break;
		default:
			break;
		}
		return h;
	}
	inline double GetDouble()
	{
		return x.d;
	}
	inline long long GetLongLong()
	{
		return x.l;
	}
	inline Data::Object* GetObj()
	{
		return x.obj;
	}
	inline void SetF(int f)
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
			bRet = (x.obj == nullptr);
			break;
		default:
			break;
		}
		return bRet;
	}
	inline ValueType GetType() { return t; }
	inline int GetF() { return flags; }
	inline void operator = (const Value& v)
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
			x.str = v.x.str;
			ChangeToStrObject();
			break;
		case ValueType::Object:
			AssignObject(v.x.obj);
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
