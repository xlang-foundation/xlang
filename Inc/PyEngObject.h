#ifndef _PyEngObject_H_
#define _PyEngObject_H_

#include <string>
#include <vector>
#include <map>
#include <unordered_map>
#include "PyEngHost.h"
#include "value.h"
#include "xlang.h"

namespace PyEng{

template<class TBase>
class ObjectRef
{
	enum class IndexType
	{
		Index,
		KEY
	};
public:
	ObjectRef(TBase p, const char* key)
	{
		m_p = p;
		m_key = key;
		m_idxType = IndexType::KEY;
	}
	ObjectRef(TBase p, int index)
	{
		m_p = p;
		m_index = index;
		m_idxType = IndexType::Index;
	}
	FORCE_INLINE ObjectRef<TBase> operator[](const char* key)
	{
		return TBase(*this).operator[](key);
	}
	template<class TFrom>
	void operator = (TFrom v)
	{
		if (m_idxType == IndexType::KEY)
		{
			g_pPyHost->KVSet(m_p,(TBase)m_key, (TBase)v);
		}
		else if (m_idxType == IndexType::Index)
		{
			g_pPyHost->Set(m_p,m_index, (TBase)v);
		}
	}
	template<typename TC>
	explicit operator TC /*const*/ () const//todo:c++17 can't take this 
	{
		if (m_idxType == IndexType::KEY)
		{
			return (TC)(TBase)g_pPyHost->Get(m_p, m_key.c_str());
		}
		else
		{
			return (TC)(TBase)g_pPyHost->Get(m_p,m_index);
		}
	}

	operator TBase () const
	{
		if (m_idxType == IndexType::KEY)
		{
			return (TBase)g_pPyHost->Get(m_p, m_key.c_str());
		}
		else
		{
			return (TBase)g_pPyHost->Get(m_p, m_index);
		}
	}

	template<typename... VarList>
	TBase operator()(VarList... args)
	{
		PyEngObjectPtr* ptrs = nullptr;
		const int size = sizeof...(args);
		TBase objs[size] = { args... };
		ptrs = new PyEngObjectPtr[size];
		for (int i = 0; i < size; i++)
		{
			ptrs[i] = objs[i];
		}
		//ptrs will be deleted by .Call
		return TBase(*this).Call(size, ptrs);
	}
	TBase operator()()
	{
		return TBase(*this).Call(0, nullptr);
	}

protected:
	TBase m_p;
	IndexType m_idxType = IndexType::Index;
	std::string m_key;
	int m_index=0;
};

//copy from C:\Python38\Lib\site-packages\numpy\core\include\numpy\ndarraytypes.h
enum class JIT_DATA_TYPES
{
	BOOL = 0,
	BYTE, UBYTE,
	SHORT, USHORT,
	INT, UINT,
	LONG, ULONG,
	LONGLONG, ULONGLONG,
	FLOAT, DOUBLE, LONGDOUBLE,
	CFLOAT, CDOUBLE, CLONGDOUBLE,
	OBJECT = 17,
	STRING, _UNICODE_,
	VOIDTYPE,
	DATAFRAME,//for DataFrame
};
enum class Action
{
	None,
	AddRef,
	ConvertToPythonObject
};
class MetaObject
{
public:
	MetaObject()
	{

	}
public:
	void SetProxyObj(void* p)
	{
		_proxy_obj_ = p;
	}
	void* GetProxyObj()
	{
		return _proxy_obj_;
	}
private:
	void* _proxy_obj_ = nullptr;
};
class Object
{
public:
	Object()
	{
	}
	static void SetTrace(Python_TraceFunc func,
		PyEngObjectPtr args)
	{
		if (g_pPyHost)
		{
			g_pPyHost->SetTrace(func, args);
		}
	}
	FORCE_INLINE static Object Import(const char* moduleName)
	{
		return g_pPyHost->Import(moduleName);
	}
	FORCE_INLINE static Object FromGlobals()
	{
		return Object(g_pPyHost->GetGlobals(),true);
	}
	FORCE_INLINE static Object FromLocals()
	{
		return Object(g_pPyHost->GetLocals(),true);
	}
	Object(const char* buf, long long size)
	{
		m_p = g_pPyHost->CreateByteArray(buf, size);
	}
	Object(X::Value& v)
	{
		Assign(v);
	}
	FORCE_INLINE void Assign(X::Value& v)
	{
		m_p = g_pPyHost->from_xvalue(v);
	}
	template <typename VALUE>
	Object(std::vector<VALUE> li)
	{
		m_p = g_pPyHost->NewList(li.size());
		for (int i=0;i<(int)li.size();i++)
		{
			g_pPyHost->Set(m_p,i,(Object)li[i]);
		}
	}
	template <typename KEY, typename VALUE>
	Object(std::map <KEY, VALUE> kvMap)
	{
		m_p = g_pPyHost->NewDict();
		for (auto kv : kvMap)
		{
			g_pPyHost->KVSet(m_p,(Object)kv.first,(Object)kv.second);
		}
	}
	template <typename VALUE>
	Object(X::Port::StringMap<VALUE> kvMap)
	{
		m_p = g_pPyHost->NewDict();
		for (auto kv : kvMap)
		{
			g_pPyHost->KVSet(m_p, (Object)kv.key, (Object)kv.val);
		}
	}
	template <typename KEY, typename VALUE>
	Object(std::unordered_map <KEY, VALUE> kvMap)
	{
		m_p = g_pPyHost->NewDict();
		for (auto kv : kvMap)
		{
			g_pPyHost->Set(m_p, (Object)kv.first, (Object)kv.second);
		}
	}
	//p is new ref which will call release on deconstructor
	Object(PyEngObjectPtr p)
	{
		m_p = p;
	}
	Object(PyEngObjectPtr p, bool bAddRef)
	{
		m_p = p;
		if (bAddRef && m_p != nullptr)
		{
			g_pPyHost->AddRef(m_p);
		}
	}
	//p below, caller will release its refcount, so Object
	//need to add itself's
	Object(PyEngObjectPtr p, Action act)
	{
		m_p = p;
		switch (act)
		{
		case PyEng::Action::AddRef:
			if (m_p != nullptr)
			{
				g_pPyHost->AddRef(m_p);
			}
			break;
		case PyEng::Action::ConvertToPythonObject:
			if(p!=nullptr)
			{
				MetaObject* pMetaObj = reinterpret_cast<MetaObject*>(p);
				if (pMetaObj)
				{

				}
			}
			break;
		default:
			break;
		}
	}

	Object(int v)
	{
		m_p = g_pPyHost->from_int(v);
	}
	Object(unsigned int v)
	{
		m_p = g_pPyHost->from_int((int)v);
	}
	Object(long long v)
	{
		m_p = g_pPyHost?g_pPyHost->from_longlong(v):nullptr;
	}
	Object(unsigned long long v)
	{
		m_p = g_pPyHost?g_pPyHost->from_longlong((long long)v):nullptr;
	}
	Object(float v)
	{
		m_p = g_pPyHost->from_float(v);
	}
	Object(double v)
	{
		m_p = g_pPyHost->from_double(v);
	}
	Object(const char* v)
	{
		m_p = g_pPyHost->from_str(v);
	}
	Object(const std::string& v)
	{
		m_p = g_pPyHost->from_str(v.c_str());
	}
	Object(const Object& self)
	{
		m_p = self.m_p;
		if (m_p)
		{
			g_pPyHost->AddRef(m_p);
		}
	}
	Object& operator=(const Object& o)
	{
		if (m_p)
		{
			g_pPyHost->Release(m_p);
		}
		m_p = o.m_p;
		if (m_p)
		{
			g_pPyHost->AddRef(m_p);
		}
		return *this;
	}
	virtual ~Object()
	{
		if (m_p)
		{
			g_pPyHost->Release(m_p);
		}
	}
	const char* GetType()
	{
		if (m_p)
		{
			return g_pPyHost->GetObjectType(m_p);
		}
		else
		{
			return "NULL";
		}
	}
	FORCE_INLINE operator PyEngObjectPtr () const
	{
		if (m_p)
		{
			g_pPyHost->AddRef(m_p);
		}
		return m_p;
	}
	FORCE_INLINE void* ref()
	{
		return m_p;
	}
	FORCE_INLINE ObjectRef<Object> operator[](int i)
	{
		return ObjectRef<Object>(m_p, i);
	}

	FORCE_INLINE ObjectRef<Object> operator[](const char* key)
	{
		return ObjectRef<Object>(m_p,key);
	}
	FORCE_INLINE bool ContainKey(const char* key)
	{
		return g_pPyHost->ContainKey(m_p, Object(key));
	}
	long long GetCount()
	{
		return g_pPyHost->GetCount(m_p);
	}
	operator int() const 
	{ 
		return g_pPyHost->to_int(m_p);
	}
	operator long long() const
	{
		return g_pPyHost->to_longlong(m_p);
	}
	operator unsigned long long() const
	{
		return (unsigned long long)g_pPyHost->to_longlong(m_p);
	}
	operator float() const
	{
		return g_pPyHost->to_float(m_p);
	}
	operator double() const
	{
		return g_pPyHost->to_double(m_p);
	}
	operator bool() const
	{
		return (bool)g_pPyHost->to_int(m_p);
	}
	operator std::string() const
	{
		auto sz =  g_pPyHost->to_str(m_p);
		std::string str(sz);
		g_pPyHost->Free(sz);
		return str;
	}
	FORCE_INLINE bool IsNull()
	{
		return (m_p == nullptr)?true: g_pPyHost->IsNone(m_p);
	}
	FORCE_INLINE bool IsBool()
	{
		return m_p == nullptr ? false : g_pPyHost->IsBool(m_p);
	}
	FORCE_INLINE bool IsLong()
	{
		return m_p == nullptr ? false : g_pPyHost->IsLong(m_p);
	}
	FORCE_INLINE bool IsDouble()
	{
		return m_p == nullptr ? false : g_pPyHost->IsDouble(m_p);
	}
	FORCE_INLINE bool IsString()
	{
		return m_p == nullptr ? false : g_pPyHost->IsString(m_p);
	}
	FORCE_INLINE bool IsDict()
	{
		return m_p == nullptr ? false : g_pPyHost->IsDict(m_p);
	}
	FORCE_INLINE bool IsArray()
	{
		return m_p == nullptr ? false : g_pPyHost->IsArray(m_p);
	}
	FORCE_INLINE bool IsList()
	{
		return m_p==nullptr?false:g_pPyHost->IsList(m_p);
	}
	FORCE_INLINE Object Call(int size, PyEngObjectPtr* ptrs)
	{
		return g_pPyHost->Call(m_p, size, ptrs);
	}
	FORCE_INLINE Object Call(int size, PyEngObjectPtr* ptrs, PyEngObjectPtr kwargs)
	{
		return g_pPyHost->Call(m_p, size, ptrs, kwargs);
	}
	FORCE_INLINE Object Call(PyEngObjectPtr args,PyEngObjectPtr kwargs)
	{
		return g_pPyHost->Call(m_p,args,kwargs);
	}

	template<typename... VarList>
	Object operator()(VarList... args)
	{
		const int size = sizeof...(args);
		Object objs[size] = { args... };
		PyEngObjectPtr pps[size];
		for (int i = 0; i < size; i++)
		{
			pps[i] = objs[i].m_p;
		}
		return Call(size, pps);
	}
	Object operator()()
	{
		return Call(0, nullptr);
	}

	void Empty()
	{
		if (m_p)
		{
			g_pPyHost->Release(m_p);
			m_p = nullptr;
		}
	}
	char* Data()
	{
		return m_p?(char*)g_pPyHost->GetDataPtr(m_p):nullptr;
	}
protected:
	PyEngObjectPtr m_p = nullptr;
};

class None:
	public Object
{
public:
	None():
		Object()
	{
		m_p = g_pPyHost->GetPyNone();
	}
	~None() //make sure to call Object's deconstructor
	{
	}
};
class Tuple :
	public Object
{
public:
	Tuple() :
		Object()
	{
		m_p = g_pPyHost->NewTuple(0);
	}
	~Tuple() 
	{
		//for tuple, the release will auto release its children
		//'s refcount
		g_pPyHost->Release(m_p);
		//todo: check 
		//make sure to call Object's deconstructor
		//g_pPyHost->CallReleaseForTupleItems(m_p);
	}
	Tuple(long long size) :
		Object()
	{
		m_p = g_pPyHost->NewTuple(size);
	}
	template <typename VALUE>
	Tuple(std::vector<VALUE> li)
	{
		m_p = g_pPyHost->NewTuple(li.size());
		for (unsigned long long i = 0; i < li.size(); i++)
		{
			Object obj(li[i]);
			//don't keep refcount in tuple, will not call release according Python ref count rules
			g_pPyHost->Set(m_p, (int)i, obj);
		}
	}
};
class Dict :
	public Object
{
public:
	Dict() :
		Object()
	{
		m_p = g_pPyHost->NewDict();
	}
	~Dict() //make sure to call Object's deconstructor
	{
	}
	Dict(const Object& self)
	{
		if (g_pPyHost->IsDict(self))
		{
			//will addref
			m_p = (PyEngObjectPtr)self;
		}
		else
		{
			m_p = nullptr;
		}
	}
	Dict& operator=(const Object& o)
	{
		if (m_p)
		{
			g_pPyHost->Release(m_p);
		}
		if (g_pPyHost->IsDict(o))
		{
			//will addref
			m_p = (PyEngObjectPtr)o;
		}
		else
		{
			m_p = nullptr;
		}
		return *this;
	}
	bool Contain(const char* key)
	{
		return g_pPyHost->DictContain(m_p, key);
	}
	bool Enum(long long& pos, Object& key, Object& val)
	{
		PyEngObjectPtr ptrKey = nullptr;
		PyEngObjectPtr ptrVal = nullptr;
		bool bOK = g_pPyHost->EnumDictItem(m_p, pos, ptrKey, ptrVal);
		if (bOK)
		{
			key = Object(ptrKey);
			val = Object(ptrVal);
		}
		return bOK;
	}
	Object Keys()
	{
		return (Object)g_pPyHost->GetDictKeys(m_p);
	}
	Object ToList()
	{
		return (Object)g_pPyHost->GetDictItems(m_p);
	}
};

template<typename ItemData_Type>
class Array:
	public Object
{
public:
	Array() 
		:Object()
	{

	}
	~Array() //make sure to call Object's deconstructor
	{
	}
	Array(const Object& obj)
		:Object(obj)
	{
		m_data = (ItemData_Type*)g_pPyHost->GetDataPtr(m_p);
		int itemType = 0;
		X::Port::vector<unsigned long long> dims;
		X::Port::vector<unsigned long long> strides;
		g_pPyHost->GetDataDesc(m_p, itemType, m_itemsize,dims, strides);
		for (auto d:dims)
		{
			m_dims.push_back(d);
		}
		for (auto d : strides)
		{
			m_strides.push_back(d);
		}
		m_itemdatatype = (JIT_DATA_TYPES)itemType;
		int a = 1;
		m_dimProd.push_back(a);
		for (int i = m_dims.size() - 1; i >= 1; i--)
		{
			a *= (int)m_dims[i];
			m_dimProd.insert(m_dimProd.begin(), a);
		}
		m_size = a * m_dims[0];
	}
	Array(int nd, unsigned long long* dims)
	{
		for (int i = 0; i < nd; i++)
		{
			m_dims.push_back(dims[i]);
		}
		int a = 1;
		m_dimProd.push_back(a);
		for (int i = nd - 1; i >= 1; i--)
		{
			a *= (int)dims[i];
			m_dimProd.insert(m_dimProd.begin(), a);
		}
		m_size = a * m_dims[0];
		SetItemType();
		m_p = g_pPyHost->NewArray(nd, dims, (int)m_itemdatatype,nullptr);
		int itemType = 0;
		X::Port::vector<unsigned long long> vecDims(0);
		X::Port::vector<unsigned long long> vecStrides(0);
		g_pPyHost->GetDataDesc(m_p, itemType, m_itemsize, vecDims, vecStrides);
		for (auto d : vecDims)
		{
			m_dims.push_back(d);
		}
		for (auto d : vecStrides)
		{
			m_strides.push_back(d);
		}
		m_data = (ItemData_Type*)g_pPyHost->GetDataPtr(m_p);
	}
	Array(int nd, unsigned long long* dims,int itemDataType)
	{
		for (int i = 0; i < nd; i++)
		{
			m_dims.push_back(dims[i]);
		}
		int a = 1;
		m_dimProd.push_back(a);
		for (int i = nd - 1; i >= 1; i--)
		{
			a *= (int)dims[i];
			m_dimProd.insert(m_dimProd.begin(), a);
		}
		m_size = a * m_dims[0];
		m_itemdatatype = (JIT_DATA_TYPES)itemDataType;
		m_p = g_pPyHost->NewArray(nd, dims, (int)m_itemdatatype,nullptr);
		int itemType = 0;
		X::Port::vector<unsigned long long> vecDims(0);
		X::Port::vector<unsigned long long> vecStrides(0);
		g_pPyHost->GetDataDesc(m_p, itemType, m_itemsize, vecDims, vecStrides);
		for (auto d : vecDims)
		{
			m_dims.push_back(d);
		}
		for (auto d : vecStrides)
		{
			m_strides.push_back(d);
		}
		m_data = (ItemData_Type*)g_pPyHost->GetDataPtr(m_p);
	}
	template<typename... index>
	FORCE_INLINE ItemData_Type& operator()(index... i)
	{
		const int size = sizeof...(i);
		long long idx[size] = { i... };

		int addr = idx[0] * m_dimProd[0];
		for (int i = 1; i < m_dimProd.size(); i++)
		{
			addr += idx[i] * m_dimProd[i];
		}
		return m_data[addr];
	}
	ItemData_Type* GetData()
	{
		return m_data;
	}
	std::vector<unsigned long long>& GetDims()
	{
		return m_dims;
	}
	unsigned long long SizeOfBytes()
	{
		return m_size* m_itemsize;
	}
	unsigned long long Size()
	{
		return m_size;
	}
	JIT_DATA_TYPES ItemType() 
	{
		return m_itemdatatype;
	}
	int ItemSize()
	{
		return m_itemsize;
	}
	int nd()
	{
		return m_dims.size();
	}
protected:
	void SetItemType();
	std::vector<int> m_dimProd;
	JIT_DATA_TYPES m_itemdatatype;
	int m_itemsize;
	std::vector<unsigned long long> m_dims;
	std::vector<unsigned long long> m_strides;
	ItemData_Type* m_data;
	unsigned long long m_size = 0;
};
template<>
FORCE_INLINE void Array<char>::SetItemType()
{
	m_itemdatatype = JIT_DATA_TYPES::BYTE;
}

template<>
FORCE_INLINE void Array<float>::SetItemType()
{
	m_itemdatatype = JIT_DATA_TYPES::FLOAT;
}
template<>
FORCE_INLINE void Array<double>::SetItemType()
{
	m_itemdatatype = JIT_DATA_TYPES::DOUBLE;
}
template<>
FORCE_INLINE void Array<int>::SetItemType()
{
	m_itemdatatype = JIT_DATA_TYPES::INT;
}

FORCE_INLINE Object Get(const char* key)
{
	return g_pPyHost->Get(nullptr,key);
}

template <class Native_Class,bool hasPythonProxy>
Object Extract(PyEngObjectPtr self,const char* class_name,Native_Class* pNativeObj)
{
	if (hasPythonProxy)
	{
		MetaObject* pMetaObject = dynamic_cast<MetaObject*>(pNativeObj);
		void* pNativePointer = pMetaObject->GetProxyObj();
		if (pNativePointer != nullptr)
		{
			return Object(pNativePointer);
		}
		else
		{
			return Object(pNativeObj, class_name);
		}
	}
	else
	{
		return Object();// g_pPyHost->QueryOrCreate(self, class_name, pNativeObj);
	}
}

};//End Namespace PyEng
#endif //_PyEngObject_H_
