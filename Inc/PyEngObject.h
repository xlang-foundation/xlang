#ifndef _PyEngObject_H_
#define _PyEngObject_H_

#include <string>
#include <vector>
#include <map>
#include <unordered_map>
#include "PyEngHost.h"

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
	inline ObjectRef<TBase> operator[](const char* key)
	{
		return TBase(*this).operator[](key);
	}
	template<class TFrom>
	void operator = (TFrom v)
	{
		if (m_idxType == IndexType::KEY)
		{
			g_pHost->KVSet(m_p,(TBase)m_key, (TBase)v);
		}
		else if (m_idxType == IndexType::Index)
		{
			g_pHost->Set(m_p,m_index, (TBase)v);
		}
	}
	template<typename TC>
	explicit operator TC /*const*/ () const//todo:c++17 can't take this 
	{
		if (m_idxType == IndexType::KEY)
		{
			return (TC)(TBase)g_pHost->Get(m_p, m_key.c_str());
		}
		else
		{
			return (TC)(TBase)g_pHost->Get(m_p,m_index);
		}
	}

	operator TBase () const
	{
		if (m_idxType == IndexType::KEY)
		{
			return (TBase)g_pHost->Get(m_p, m_key.c_str());
		}
		else
		{
			return (TBase)g_pHost->Get(m_p, m_index);
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
	static Object Import(const char* moduleName)
	{
		return g_pHost->Import(moduleName);
	}
	Object(const char* buf, long long size)
	{
		m_p = g_pHost->CreateByteArray(buf, size);
	}
	template <typename VALUE>
	Object(std::vector<VALUE> li)
	{
		m_p = g_pHost->NewList(li.size());
		for (int i=0;i<(int)li.size();i++)
		{
			g_pHost->Set(m_p,i,(Object)li[i]);
		}
	}
	template <typename KEY, typename VALUE>
	Object(std::map <KEY, VALUE> kvMap)
	{
		m_p = g_pHost->NewDict();
		for (auto kv : kvMap)
		{
			g_pHost->KVSet(m_p,(Object)kv.first,(Object)kv.second);
		}
	}
	template <typename KEY, typename VALUE>
	Object(std::unordered_map <KEY, VALUE> kvMap)
	{
		m_p = g_pHost->NewDict();
		for (auto kv : kvMap)
		{
			g_pHost->Set(m_p, (Object)kv.first, (Object)kv.second);
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
			g_pHost->AddRef(m_p);
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
				g_pHost->AddRef(m_p);
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
		m_p = g_pHost->from_int(v);
	}
	Object(unsigned int v)
	{
		m_p = g_pHost->from_int((int)v);
	}
	Object(long long v)
	{
		m_p = g_pHost->from_longlong(v);
	}
	Object(unsigned long long v)
	{
		m_p = g_pHost->from_longlong((long long)v);
	}
	Object(float v)
	{
		m_p = g_pHost->from_float(v);
	}
	Object(double v)
	{
		m_p = g_pHost->from_double(v);
	}
	Object(const char* v)
	{
		m_p = g_pHost->from_str(v);
	}
	Object(const std::string& v)
	{
		m_p = g_pHost->from_str(v.c_str());
	}
	Object(const Object& self)
	{
		m_p = self.m_p;
		if (m_p)
		{
			g_pHost->AddRef(m_p);
		}
	}
	Object& operator=(const Object& o)
	{
		if (m_p)
		{
			g_pHost->Release(m_p);
		}
		m_p = o.m_p;
		if (m_p)
		{
			g_pHost->AddRef(m_p);
		}
		return *this;
	}
	~Object()
	{
		if (m_p)
		{
			g_pHost->Release(m_p);
		}
	}
	const char* GetType()
	{
		if (m_p)
		{
			return g_pHost->GetObjectType(m_p);
		}
		else
		{
			return "NULL";
		}
	}
	inline operator PyEngObjectPtr () const
	{
		if (m_p)
		{
			g_pHost->AddRef(m_p);
		}
		return m_p;
	}
	inline void* ref()
	{
		return m_p;
	}
	inline ObjectRef<Object> operator[](int i)
	{
		return ObjectRef<Object>(m_p, i);
	}

	inline ObjectRef<Object> operator[](const char* key)
	{
		return ObjectRef<Object>(m_p,key);
	}
	inline bool ContainKey(const char* key)
	{
		return g_pHost->ContainKey(m_p, Object(key));
	}
	long long GetCount()
	{
		return g_pHost->GetCount(m_p);
	}
	operator int() const 
	{ 
		return g_pHost->to_int(m_p);
	}
	operator long long() const
	{
		return g_pHost->to_longlong(m_p);
	}
	operator unsigned long long() const
	{
		return (unsigned long long)g_pHost->to_longlong(m_p);
	}
	operator float() const
	{
		return g_pHost->to_float(m_p);
	}
	operator double() const
	{
		return g_pHost->to_double(m_p);
	}
	operator bool() const
	{
		return (bool)g_pHost->to_int(m_p);
	}
	operator std::string() const
	{
		auto sz =  g_pHost->to_str(m_p);
		std::string str(sz);
		g_pHost->Free(sz);
		return str;
	}
	inline bool IsNull()
	{
		return (m_p == nullptr)?true: g_pHost->IsNone(m_p);
	}
	inline bool IsDict()
	{
		return m_p == nullptr ? false : g_pHost->IsDict(m_p);
	}
	inline bool IsArray()
	{
		return m_p == nullptr ? false : g_pHost->IsArray(m_p);
	}
	inline bool IsList()
	{
		return m_p==nullptr?false:g_pHost->IsList(m_p);
	}
	inline Object Call(int size, PyEngObjectPtr* ptrs)
	{
		return g_pHost->Call(m_p, size, ptrs);
	}
	inline Object Call(int size, PyEngObjectPtr* ptrs, PyEngObjectPtr kwargs)
	{
		return g_pHost->Call(m_p, size, ptrs, kwargs);
	}
	inline Object Call(PyEngObjectPtr args,PyEngObjectPtr kwargs)
	{
		return g_pHost->Call(m_p,args,kwargs);
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
			g_pHost->Release(m_p);
			m_p = nullptr;
		}
	}
	char* Data()
	{
		return m_p?(char*)g_pHost->GetDataPtr(m_p):nullptr;
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
		m_p = g_pHost->GetPyNone();
	}
};
class Tuple :
	public Object
{
public:
	Tuple() :
		Object()
	{
		m_p = g_pHost->NewTuple(0);
	}
	Tuple(long long size) :
		Object()
	{
		m_p = g_pHost->NewTuple(size);
	}
	template <typename VALUE>
	Tuple(std::vector<VALUE> li)
	{
		m_p = g_pHost->NewTuple(li.size());
		for (unsigned long long i = 0; i < li.size(); i++)
		{
			g_pHost->Set(m_p, (int)i, (Object)li[i]);
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
		m_p = g_pHost->NewDict();
	}
	Dict(const Object& self)
	{
		if (g_pHost->IsDict(self))
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
			g_pHost->Release(m_p);
		}
		if (g_pHost->IsDict(o))
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
		std::string strKey(key);
		return g_pHost->DictContain(m_p, strKey);
	}
	Object Keys()
	{
		return (Object)g_pHost->GetDictKeys(m_p);
	}
	Object ToList()
	{
		return (Object)g_pHost->GetDictItems(m_p);
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
	Array(const Object& obj)
		:Object(obj)
	{
		m_data = (ItemData_Type*)g_pHost->GetDataPtr(m_p);
		int itemType = 0;
		g_pHost->GetDataDesc(m_p, itemType, m_itemsize,
			m_dims, m_strides);
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
		m_p = g_pHost->NewArray(nd, dims, (int)m_itemdatatype);
		int itemType = 0;
		g_pHost->GetDataDesc(m_p, itemType, m_itemsize,
			m_dims, m_strides);
		m_data = (ItemData_Type*)g_pHost->GetDataPtr(m_p);
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
		m_p = g_pHost->NewArray(nd, dims, (int)m_itemdatatype);
		int itemType = 0;
		g_pHost->GetDataDesc(m_p, itemType, m_itemsize,
			m_dims, m_strides);
		m_data = (ItemData_Type*)g_pHost->GetDataPtr(m_p);
	}
	template<typename... index>
	inline ItemData_Type& operator()(index... i)
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
inline void Array<char>::SetItemType()
{
	m_itemdatatype = JIT_DATA_TYPES::BYTE;
}

template<>
inline void Array<float>::SetItemType()
{
	m_itemdatatype = JIT_DATA_TYPES::FLOAT;
}
template<>
inline void Array<double>::SetItemType()
{
	m_itemdatatype = JIT_DATA_TYPES::DOUBLE;
}
template<>
inline void Array<int>::SetItemType()
{
	m_itemdatatype = JIT_DATA_TYPES::INT;
}

inline Object Get(const char* key)
{
	return g_pHost->Get(nullptr,key);
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
		return nullptr;// g_pHost->QueryOrCreate(self, class_name, pNativeObj);
	}
}

};//End Namespace PyEng
#endif //_PyEngObject_H_
