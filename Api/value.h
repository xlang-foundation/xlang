#pragma once

//ABI check,string used here just for local usage, not cross ABI
#include <string>
#include "xport.h"

namespace X 
{
class XObj;
class XLStream;
class XPackage;
enum class ValueType
{
	Invalid,
	None,
	Int64,
	Double,
	Object,
	Str,
	Value,
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
			(*((XObj*)x.obj)) op v;\
		}\
		break;\
	default:\
		break;\
	}\
	return *this;\
}

#define COMPARE_OP(op)\
bool operator op (const Value& r) const;

#define COMPARE_OP_IMPL(op)\
bool Value::operator op (const Value& r) const\
{\
	bool bRet = false;\
	switch (t)\
	{\
	case ValueType::Invalid:\
		bRet = (r.t op ValueType::None);\
		break;\
	case ValueType::None:\
		bRet = (r.t op ValueType::None);\
		break;\
	case ValueType::Int64:\
		bRet = (x.l op ToInt64(r));\
		break;\
	case ValueType::Double:\
		bRet = (x.d op ToDouble(r));\
		break;\
	case ValueType::Object:\
		bRet = (x.obj->cmp((Value*)&r) op 0);\
		break;\
	case ValueType::Str:\
		if(r.t == ValueType::Object)\
		{\
			bRet = (0 op r.x.obj->cmp((Value*)this));\
		}\
		else if(r.t == ValueType::Str)\
		{\
			bRet = (ToStr(x.str,flags) op ToStr(r.x.str,r.flags));\
		}\
		break;\
	default:\
		break;\
	}\
	return bRet;\
}

//review 4/10/2023
//ABI issue with Value, different compiler may generate different layout of members
//TODO:change to use struct to wrap its members

enum class ValueSubType
{
	NONE = 0,//if as this, means value's type is ValueType, no sub type
	BOOL = 1,
	CHAR = 2,
	UCHAR =3,
	SHORT =4,
	USHORT =5,
	INT = 8,
	UINT =9,
	FLOAT =10,
};
class Value
{
	//from high->low, second and third byte are digits number , mask is 0x00FFFF00 and shift >>8
	//last 4 bits as subtype, mask is 0x0F
	int flags = 0;

	ValueType t= ValueType::Invalid;
	union
	{
		long long l;
		double d;
		char* str;
		XObj* obj;
	}x;
	Value QueryMember(const char* key);
public:
	inline bool IsLong() { return t == ValueType::Int64; }
	inline bool IsDouble() { return t == ValueType::Double; }
	inline bool IsNumber() { return IsLong() || IsDouble();}
	inline bool IsBool() { return IsLong() && (flags & (int)ValueSubType::BOOL); }
	inline bool IsInvalid()
	{
		return (t == ValueType::Invalid);
	}
	inline bool IsValid()
	{
		return (t != ValueType::Invalid);
	}
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
	inline void Clear()
	{
		if (t == ValueType::Object)
		{
			ReleaseObject(x.obj);
		}
		t = ValueType::Invalid;
	}
	inline Value()
	{
		t = ValueType::Invalid;
		x.l = 0;
	}
	inline Value(ValueType t0)
	{
		t = t0;
	}
	inline Value(bool b)
	{//use 1 as true and 0 as false, set flag to -1
		t = ValueType::Int64;
		flags |= (int)ValueSubType::BOOL;
		x.l = b ? 1 : 0;
	}
	inline void AsBool()
	{
		flags |= (int)ValueSubType::BOOL;
		x.l = x.l>0?1 : 0;
	}
	inline Value(char c)
	{
		t = ValueType::Int64;
		flags |= (int)ValueSubType::CHAR;
		x.l = c;
	}
	inline Value(int l)
	{
		t = ValueType::Int64;
		x.l = l;
	}
	inline Value(unsigned int l)
	{
		t = ValueType::Int64;
		x.l = l;
	}
	inline Value(long l)
	{
		t = ValueType::Int64;
		x.l = l;
	}
	inline Value(unsigned long l)
	{
		t = ValueType::Int64;
		x.l = l;
	}
	inline Value(long long l)
	{
		t = ValueType::Int64;
		x.l = l;
	}

	inline Value(void* pointer)
	{
		t = ValueType::Int64;
		x.l = (long long)pointer;
	}
	inline Value(unsigned long long l)
	{
		t = ValueType::Int64;
		x.l = (long long)l;
	}
	inline Value(double d)
	{
		t = ValueType::Double;
		x.d = d;
	}
	inline Value(const char* s)
	{
		t = ValueType::Str;
		flags = (int)std::string(s).length();//avoid to use strlen, need to include <string.h> in Linux
		x.str = (char*)s;
	}
	inline Value(char* s, int size)
	{
		t = ValueType::Str;
		flags = size;
		x.str = s;
	}
	inline Value(XObj* p,bool AddRef = true)
	{
		t = ValueType::Object;
		x.obj = nullptr;
		AssignObject(p, AddRef);
	}
	Value(std::string& s);
	void SetObj(XObj* p)
	{
		t = ValueType::Object;
		x.obj = p;
	}
	bool Clone();
	bool ChangeToStrObject();
	void AssignObject(XObj* p,bool bAddRef = true);
	void ReleaseObject(XObj* p);
	inline Value Negative() const
	{
		Value newV = this;
		switch (t)
		{
		case ValueType::Int64:
			newV.x.l = -x.l;
			break;
		case ValueType::Double:
			newV.x.d = -x.d;
			break;
		default://others no change
			break;
		}
		return newV;
	}
	inline Value(const Value& v)
	{
		flags = v.flags;
		x.l = 0;
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
	operator bool() const
	{
		return (x.l != 0);
	}
	operator double() const
	{
		return (t == ValueType::Int64)?(double)x.l:x.d;
	}
	operator float() const
	{
		return (t == ValueType::Int64) ? (float)x.l : (float)x.d;
	}
	operator long long() const
	{
		return (t == ValueType::Int64) ? x.l : (long long)x.d;
	}
	operator unsigned long long() const
	{
		return (t == ValueType::Int64) ? (unsigned long long)x.l : (unsigned long long)x.d;
	}
	operator int() const
	{
		return (t == ValueType::Int64) ? (int)x.l : (int)x.d;
	}
	operator unsigned int() const
	{
		return (t == ValueType::Int64) ? (unsigned int)x.l : (unsigned int)x.d;
	}
	operator unsigned long() const
	{
		return (t == ValueType::Int64) ? (unsigned long)x.l : (unsigned long)x.d;
	}
	operator std::string()
	{
		return ToString();
	}
#if 0
	template<typename toT>
	operator toT* () const;
#endif
	template<typename toT>
	operator toT* () const
	{
		return (toT*)CastObjectToPointer();
	}
	void* CastObjectToPointer() const;

	operator void* ()const
	{
		return (void*)x.obj;
	}
	
	size_t Hash();

	inline double GetDouble()
	{
		return x.d;
	}
	inline void SetDouble(double d)
	{
		x.d = d;
	}
	inline long long GetLongLong()
	{
		return x.l;
	}
	inline void SetLongLong(long long l)
	{
		x.l =l;
	}
	void SetString(std::string& s);
	inline bool GetBool()
	{
		return (x.l!=0);
	}
	inline XObj* GetObj() const
	{
		return x.obj;
	}
	inline void SetDigitNum(int num)
	{
		int bits = (num << 8) & 0x00FFFF00;
		flags |= bits;
	}
	inline int GetDigitNum()
	{
		return (flags & 0x00FFFF00) >> 8;
	}
	inline void SetF(int f)
	{
		flags = f;
	}
	inline bool IsObject() const
	{
		return (t == ValueType::Object) && (x.obj != nullptr);
	}
	bool IsList() const;
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
	std::string GetValueType();
	inline ValueType GetType() { return t; }
	inline void SetType(ValueType t0) { t = t0; }
	inline int GetF() { return flags; }
	bool CallAssignIfObjectSupport(const Value& v);
	virtual inline void operator = (const Value& v)
	{
		if (IsObject())
		{
			if (CallAssignIfObjectSupport(v))
			{
				return;
			}
			ReleaseObject(x.obj);
		}
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
	virtual Value operator* (const Value& right);
	virtual Value operator / (const Value& right);
	virtual Value operator + (const Value& right);
	virtual Value operator - (const Value& right);
	virtual void operator += (const Value& v);
	virtual void operator -= (const Value& v);

	Value ObjCall(Port::vector<X::Value>& params);
	Value ObjCall(Port::vector<X::Value>& params,Port::StringMap<X::Value>& kwParams);

	template<typename... VarList>
	Value operator()(VarList... args)
	{
		if (!IsObject())
		{
			return Value();
		}
		const int size = sizeof...(args);
		Value vals[size] = { args... };
		Port::vector<X::Value> params(size);
		for (int i = 0; i < size; i++)
		{
			params.push_back(vals[i]);
		}
		return ObjCall(params);
	}
	Value operator()()
	{
		if (!IsObject())
		{
			return Value();
		}
		Port::vector<X::Value> params(0);
		return ObjCall(params);
	}
	Value GetItemValue(long long idx);
	Value GetObjectValue(Port::vector<X::Value>& IdxAry);
	template<typename... VarList>
	Value Query(VarList... args)
	{
		const int size = sizeof...(args);
		Value vals[size] = { args... };
		Port::vector<X::Value> IdxAry(size);
		for (int i = 0; i < size; i++)
		{
			IdxAry.push_back(vals[i]);
		}
		return GetObjectValue(IdxAry);
	}
	inline Value operator[](long long index)
	{
		return GetItemValue(index);
	}
	inline Value operator[](const char* key)
	{
		return QueryMember(key);
	}
	//ARITH_OP(-= );
	ARITH_OP(*= );
	ARITH_OP(/= );
	COMPARE_OP(==);
	COMPARE_OP(!=);
	COMPARE_OP(>);
	COMPARE_OP(<);
	COMPARE_OP(>=);
	COMPARE_OP(<=);

	std::string ToString(bool WithFormat = false);
	bool FromBytes(XLStream* pStream = nullptr);
	bool ToBytes(XLStream* pStream = nullptr);
	Value getattr(const char* attrName) const;
	void setattr(const char* attrName, X::Value& attrVal) const;
	long long Size();
};
template<typename T>
class V:
	public Value
{
	template<typename... VarList>
	void Create(VarList... args);
	void Create();
public:
	V()
	{
		Create();
	}
	template<typename... VarList>
	V(VarList... args)
	{
		Create(args...);
	}
	V(const Value& v):
		Value(v)
	{

	}
	V(T* obj):
		Value(obj)
	{

	}
	operator T* ()
	{
		XObj* pObj = this->GetObj();
		return  dynamic_cast<T*>(pObj);
	}
	T* operator->()
	{
		return dynamic_cast<T*>(GetObj());
	}
};
}

