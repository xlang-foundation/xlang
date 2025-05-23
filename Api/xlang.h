﻿/*
Copyright (C) 2024 The XLang Foundation
Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

#ifndef _X_LANG_H_
#define _X_LANG_H_

#include "value.h"
#include "xhost.h"

namespace X
{
	enum class ObjType
	{
		Base,
		Type,
		Str,
		Binary,
		Expr,
		ConstExpr, //simple expression can be translate into c++
		Function,
		MetaFunction,
		XClassObject,
		Prop,
		ObjectEvent,
		FuncCalls,
		Package,
		ModuleObject,
		Future,
		TaskPool,
		Iterator,
		Range,
		List,
		Dict,
		Tensor,
		TensorExpression,
		TensorOperator,
		TensorGraph,
		Complex,
		Set,
		Struct,
		StructField,
		TableRow,
		Table,
		DeferredObject,
		RemoteObject,
		PyProxyObject,
		InternalAssign,
		Error,
		Ref
	};

	enum class MemberFlag
	{
		//Flag's first bye is for PackageMemberType
		//second byte:
		KeepRawParams = 0x00000100,
	};
	#define IS_KEEP_RAW_PARAMS(flag) ((flag & int(MemberFlag::KeepRawParams)) != 0)

	//For XPackage
	enum class PackageMemberType
	{
		None,
		Func,
		FuncEx,
		Prop,
		Const,
		ObjectEvent,
		Class,
		ClassInstance,
		Module
	};
#define Internal_Reserve(cls_name)  cls_name(int){}

	//Used for XList or other XObj's derived class to do iteration
	//used for C++ native code
	class XObj;
	typedef unsigned long long XIterator_Pos;

	template<class T>
	class Iterator 
	{
	private:
		T* mHost = nullptr;
		XIterator_Pos mPos = (XIterator_Pos)(-1);//maybe a pointer
	public:
		FORCE_INLINE Iterator(T* host, XIterator_Pos pos =0)
		{
			mHost = host;
			if (pos == (XIterator_Pos)(-1))
			{
				mPos = (XIterator_Pos)mHost->Size();
			}
			else
			{
				mPos = pos;
			}
		}

		// Overload the increment operator
		FORCE_INLINE Iterator& operator++()
		{
			mPos = mHost->IteratorAdd(mPos);
			return *this;
		}

		FORCE_INLINE Value operator*() const
		{
			return mHost->IteratorGet(mPos);
		}

		// Overload the equality operator
		FORCE_INLINE bool operator!=(const Iterator& other) const
		{
			return mPos != other.mPos;
		}
	};


	class XObj
	{
		friend class Iterator<XObj>;
		Iterator<XObj> mIterator;
	protected:
		XRuntime* m_rt = nullptr;
		XObj* m_parent = nullptr;

		FORCE_INLINE void SetParent(XObj* p)
		{
			m_parent = p;
		}
		virtual XIterator_Pos IteratorAdd(XIterator_Pos pos) { return (XIterator_Pos)(-1); }
		virtual Value IteratorGet(XIterator_Pos pos) 
		{ 
			return X::Value();
		}
	public:
		XObj() :mIterator(this)
		{

		}
		~XObj()
		{
		}
		FORCE_INLINE void SetRT(XRuntime* rt)
		{
			m_rt = rt;
		}
		void SetContext(XRuntime* rt, XObj* pa)
		{
			m_rt = rt;
			m_parent = pa;
		}
		XRuntime* RT() { return m_rt; }
		XObj* Parent() { return m_parent; }
		XObj& operator=(const XObj& o)
		{
			m_rt = o.m_rt;
			m_parent = o.m_parent;
			return *this;
		}
		FORCE_INLINE virtual bool CanSetObjectName() { return false; }
		virtual XObj* Clone() { return nullptr; }
		FORCE_INLINE virtual bool SupportAssign() { return false; }
		FORCE_INLINE virtual bool Assign(const X::Value& val) { return false; }
		virtual int QueryMethod(const char* name, int* pFlags = nullptr) { return -1; };
		virtual bool GetIndexValue(int idx, Value& v) { return false; };
		virtual bool Get(XRuntime* rt, XObj* pContext, X::Port::vector<X::Value>& IdxAry, X::Value& val) { return false; }
		virtual bool Set(Value valIdx, X::Value& val) { return false; }
		virtual int IncRef() { return 0; }
		virtual int DecRef() { return 0; }
		virtual ObjType GetType() { return ObjType::Base; }
		virtual const char* GetTypeString() { return nullptr; }
		virtual long long Size() { return 0; }
		virtual X::Value Shapes() { return X::Value(); }
		virtual size_t Hash() { return 0; }
		virtual unsigned long long GetID() { return 0; }
		virtual const char* ToString(bool WithFormat = false) 
		{
			return nullptr;
		}
		virtual bool FromString(const char* strCoded)
		{
			return true;
		}
		virtual bool Call(XRuntime* rt, XObj* pContext,
			ARGS& params,
			KWARGS& kwParams,
			X::Value& retValue)
		{
			return false;
		}
		virtual bool CallEx(XRuntime* rt, XObj* pContext,
			ARGS& params,
			KWARGS& kwParams,
			X::Value& trailer,
			X::Value& retValue)
		{
			return false;
		}
		//Ops
		virtual bool Multiply(const X::Value& r,X::Value& retVal)
		{
			return false;
		}
		virtual bool Divide(const X::Value& r, X::Value& retVal)
		{
			return false;
		}
		//for case leftValue / this_Object
		virtual bool Divided(const X::Value& leftValue, X::Value& retVal)
		{
			return false;
		}
		virtual bool Add(const X::Value& r, X::Value& retVal)
		{
			return false;
		}
		virtual bool Minus(const X::Value& r, X::Value& retVal)
		{
			return false;
		}
		//for case leftValue - this_Object
		virtual bool Minuend(const X::Value& leftValue, X::Value& retVal)
		{
			return false;
		}
		virtual XObj& operator +=(Value& r)
		{
			return *this;
		}
		virtual XObj& operator -=(Value& r)
		{
			return *this;
		}
		virtual XObj& operator *=(Value& r)
		{
			return *this;
		}
		virtual XObj& operator /=(Value& r)
		{
			return *this;
		}
		virtual int cmp(Value* r)
		{
			return 0;
		}
		//API Wrapper
		FORCE_INLINE virtual Value Member(XRuntime* rt,const char* name)
		{
			X::Value retVal = g_pXHost->QueryMember(rt,this, name);
			if (retVal.IsObject())
			{
				retVal.GetObj()->SetContext(rt, this);
			}
			return retVal;
		}
		FORCE_INLINE Value Member(const char* name)
		{
			if (m_rt == nullptr)
			{
				m_rt = g_pXHost->GetCurrentRuntime();
			}
			return Member(m_rt, name);
		}
		template<typename... VarList>
		X::Value operator()(VarList... args)
		{
			const int size = sizeof...(args);
			X::Value vals[size] = { args... };
			X::ARGS params;
			for (int i = 0; i < size; i++)
			{
				params.push_back(vals[i]);
			}
			X::KWARGS kwargs;
			X::Value v0;
			Call(m_rt,m_parent,params, kwargs,v0);
			if (v0.IsObject())
			{
				v0.GetObj()->SetContext(m_rt,m_parent);
			}
			return v0;
		}
		FORCE_INLINE X::Value operator()()
		{
			X::ARGS params(0);
			X::KWARGS kwargs;
			X::Value v0;
			Call(m_rt,m_parent, params, kwargs, v0);
			if (v0.IsObject())
			{
				v0.GetObj()->SetContext(m_rt,m_parent);
			}
			return v0;
		}
		FORCE_INLINE Iterator<XObj> begin()
		{
			return Iterator<XObj>(this);
		}
		FORCE_INLINE Iterator<XObj> end()
		{
			return Iterator<XObj>(this,-1);
		}
	};
	class XRuntime :
		virtual public XObj
	{
	public:
		virtual bool CreateEmptyModule() = 0;
		virtual X::Value GetXModuleFileName() = 0;
		virtual int GetTopStackCurrentLine() = 0;
		virtual bool AddVar(const char* name, X::Value& val) = 0;
		virtual X::Value GetModuleObject() = 0;
	};
	class XModule :
		virtual public XObj
	{
	public:
		virtual const char* GetFileName() = 0;
		virtual const char* GetPath() = 0;
		virtual XRuntime* GetRT() = 0;
	};
	class XConstExpr :
		virtual public XObj
	{
	public:
		Internal_Reserve(XConstExpr)
	};
	class XStr :
		virtual public XObj
	{
	public:
		Internal_Reserve(XStr)
		virtual char* Buffer() = 0;
	};
	class XError :
		virtual public XObj
	{
	public:
		virtual const char* GetInfo() = 0;
		virtual int GetCode() = 0;
	};
	class XIterator :
		virtual public XObj
	{
	public:
		Internal_Reserve(XIterator)
	};
	class XList :
		virtual public XObj
	{
	public:
		Internal_Reserve(XList)
		virtual Value Get(long long idx) = 0;
		virtual void AddItem(X::Value& v) = 0;
		FORCE_INLINE void append(X::Value& v)
		{
			AddItem(v);
		}
	};
	class XDict :
		virtual public XObj
	{
	public:
		using Dict_Enum = X::Port::Function<void(X::Value& key, X::Value& val)>;
		Internal_Reserve(XDict)
		virtual void Set(const X::Value& key, const X::Value& val) = 0;
		virtual X::Value Get(const X::Value& key) = 0;
		virtual bool Has(const X::Value& key) = 0;
		virtual void Enum(Dict_Enum proc) = 0;
		virtual bool Compare(X::Value& dict) = 0;
		FORCE_INLINE void Set(const char* key, X::Value val)
		{
			X::Value k(key);
			Set(k,val);
		}
		virtual Value& operator[](X::Value& key) = 0;
		FORCE_INLINE Value& operator[](const char* key)
		{
			X::Value k(key);
			return operator[](k);
		}
	};
	class XSet :
		virtual public XObj
	{
	public:
		Internal_Reserve(XSet)
	};
	class XStruct :
		virtual public XObj
	{
	public:
		Internal_Reserve(XStruct)
		virtual void addField(
			const char* name,const char* type, bool isPointer = false, int bits = 0) = 0;
		virtual bool Build() = 0;
		virtual char* Data() = 0;
	};

	enum class TensorDataType 
	{
		BOOL = 0,

		BYTE = 1,
		UBYTE = 2,

		SHORT = 3,
		USHORT = 4,

		INT = 5,
		INT32 = INT,
		UINT = 6,
		UINT32 = UINT,

		LONGLONG = 7,
		INT64 = LONGLONG,

		ULONGLONG = 8,
		UINT64 = ULONGLONG,

		BFLOAT16 = 9,

		HALFFLOAT = 10,
		FLOAT16 = HALFFLOAT,

		FLOAT = 11,
		FLOAT32 = FLOAT,

		DOUBLE = 12,
		FLOAT64 = DOUBLE,

		CFLOAT = 13,
		COMPLEX64 = CFLOAT,

		CDOUBLE = 14,
		COMPLEX128 = CDOUBLE,

		QINT8 = 15,
		QUINT8 = 16,
		QINT32 = 17,
		FLOAT8 = 18,
		FLOAT8_E4M3FN = 18,
		FLOAT8_E4M3FNUZ,
		FLOAT8_E5M2,
		FLOAT8_E5M2FNUZ,
		UNKNOWN
	};

	class XTensor :
		virtual public XObj
	{
	public:
		Internal_Reserve(XTensor)
		virtual long long GetDataSize() = 0;
		virtual long long GetItemSize() = 0;
		virtual char* GetData() = 0;
		virtual int GetDimCount() = 0;
		virtual long long GetCount() = 0;
		virtual long long GetDimSize(int dimIdx) = 0;
		virtual void SetShape(Port::vector<int>& shapes) = 0;
		virtual void SetDataType(TensorDataType t) = 0;
		virtual void SetData(char* data, long long size) = 0;
		virtual TensorDataType GetDataType() = 0;
		virtual bool Create(X::Value& initData) = 0;
		virtual X::Value GetDesc() = 0;
		virtual void SetDesc(X::Value& v) = 0;
		virtual X::Value GetName() = 0;
		virtual void SetName(X::Value& name) = 0;
	};
	class XTensorExpression :
		virtual public XObj
	{
	public:
		Internal_Reserve(XTensorExpression)
	};
	class XTensorOperator :
		virtual public XObj
	{
	public:
		Internal_Reserve(XTensorOperator)
		virtual void SetName(const char* n) = 0;
	};
	class XTensorGraph :
		virtual public XObj
	{
	public:
		Internal_Reserve(XTensorGraph)
		virtual void Create(XObj* pContext,X::ARGS& params, X::KWARGS& kwParams) = 0;
		virtual void PutTensorIntoCache(X::Value& vTensor) = 0;
		virtual void RemoveTensorFromCache(X::Value& vTensor) = 0;
		virtual bool Run(X::ARGS& params, X::KWARGS& kwParams) = 0;
		virtual X::Value GetCodeGenerated() = 0;
	};
	class XComplex :
		virtual public XObj
	{
	public:
		Internal_Reserve(XComplex)
	};
	class XBin :
		virtual public XObj
	{
	public:
		Internal_Reserve(XBin)
		virtual char* Data() = 0;
		virtual char* BorrowDta() = 0;//then XBin not own this data
	};
	class XProp :
		virtual public XObj
	{
	protected:
		virtual bool SetPropValue(XRuntime* rt0, XObj* pContext, Value& v) = 0;
		virtual bool GetPropValue(XRuntime* rt0, XObj* pContext, Value& v) = 0;
	public:
		FORCE_INLINE Value Get()
		{
			Value v0;
			GetPropValue(m_rt,m_parent, v0);
			return v0;
		}
		FORCE_INLINE bool Set(Value& v)
		{
			return SetPropValue(m_rt,m_parent, v);
		}
	};
	class XEvent :
		virtual public XObj
	{
	public:
		virtual void SetChangeHandler(OnEventHandlerChanged ch) = 0;
		virtual void DoFire(XRuntime* rt, XObj* pContext, ARGS& params, KWARGS& kwargs) = 0;
	};
	class XFunc:
		virtual public XObj
	{
	public:
		virtual X::Value GetName() = 0;
		virtual X::Value GetCode(bool includehead) = 0;
		virtual X::Value GetParameterNameList() = 0;
		virtual void ChangeStatmentsIntoTranslateMode(
			bool changeIfStatment,bool changeLoopStatment) = 0;
	};
	class XLangClass :
		virtual public XObj
	{
	public:
		virtual int GetBaseClassCount() = 0;
		virtual X::Value GetBaseClass(int idx) = 0;
		virtual X::Value GetXClassName() = 0;
	};
	class XPyObject :
		virtual public XObj
	{
	public:
		Internal_Reserve(XPyObject);
		virtual X::Value ToXlang() = 0;
		virtual bool GetObj(void** ppObjPtr) = 0;
	};
	class XRef :
		virtual public XObj
	{
	public:
		Internal_Reserve(XRef);
		virtual X::Value Apply() = 0;
	};

	class XRemoteObject :
		virtual public XObj
	{
	public:
		virtual void SetObjID(unsigned long pid,void* objid) = 0;
		virtual int GetMemberFlags() = 0;
	};
	class XDeferredObject :
		virtual public XObj
	{
	public:

	};
	class XCustomScope
	{
		void* m_embedScope = nullptr;
	public:
		~XCustomScope()
		{
			if (m_embedScope)
			{
				g_pXHost->DeleteScopeWrapper(this);
			}
		}
		void SetScope(void* p) { m_embedScope = p; }
		void* GetScope() { return m_embedScope; }
		virtual int AddOrGet(const char* name, bool bGetOnly) = 0;
		virtual bool Get(int idx, X::Value& v, void* lValue = nullptr) = 0;
		virtual bool Set(int idx, X::Value& v) = 0;
		virtual bool VerifyNameIndex(const char* name, int idx) = 0;
	};
	class XPackage:
		virtual public XObj
	{
	public:
		virtual void SetPackageCleanupFunc(PackageCleanup func) = 0;
		virtual void SetPackageWaitFunc(PackageWaitFunc func) = 0;
		virtual void SetPackageAccessor(PackageAccessor func) = 0;
		virtual void SetPackageCall(U_FUNC func) = 0;
		virtual int AddMember(PackageMemberType type,const char* name,const char* doc,bool keepRawParams =false) = 0;
		virtual int GetPackageName(char* buffer,int bufferSize)= 0;
		virtual void* GetEmbedObj() = 0;
		virtual void SetEmbedObj(void* p) = 0;
		virtual bool Init(int varNum) = 0;
		virtual bool SetIndexValue(int idx, Value& v) = 0;
		virtual void RemoveALl() = 0;
		virtual bool RunCodeWithThisScope(const char* code) = 0;
		virtual void SetAPISet(void* pApiSet) = 0;
		virtual bool IsSamePackage(XPackage* pPack) = 0;
	};
	FORCE_INLINE long OnEvent(const char* evtName, EventHandler handler)
	{
		return g_pXHost->OnEvent(evtName, handler);
	}
	FORCE_INLINE void OffEvent(const char* evtName, long Cookie)
	{
		return g_pXHost->OffEvent(evtName, Cookie);
	}

	template<typename T>
	class XPackageValue
	{
		T* m_obj = nullptr;
		bool m_ownObj = false;
	public:
		XPackageValue()
		{
			m_obj = new T();
			m_ownObj = true;
		}
		~XPackageValue()
		{
			if (m_ownObj && m_obj)
			{
				delete m_obj;
			}
		}
		XPackageValue(T* p) //only used in Android 
		{
			m_obj = p;
		}
		XPackageValue(X::Value& v)
		{
			if (v.IsObject())
			{
				if (v.GetObj()->GetType() == X::ObjType::RemoteObject)
				{
					X::Value nativeObj;
					if (g_pXHost->ExtractNativeObjectFromRemoteObject(v, nativeObj))
					{
						X::XPackage* pPack = dynamic_cast<X::XPackage*>(nativeObj.GetObj());
						if (pPack)
						{
							m_obj = (T*)pPack->GetEmbedObj();
						}
					}
				}
				else
				{
					X::XPackage* pPack = dynamic_cast<X::XPackage*>(v.GetObj());
					m_obj = (T*)pPack->GetEmbedObj();
				}
			}
		}
		operator Value()
		{
			if (m_obj)
			{
				auto& apiset = m_obj->APISET();
				X::XPackage* pPackage = apiset.GetProxy(m_obj);
				//only be used onece, if convert to Value, then don't do again
				//set this flag to avoid delete m_obj during deconstruction  
				m_ownObj = false;
				return Value(dynamic_cast<X::XObj*>(pPackage),false);
			}
			else
			{
				return Value();
			}
		}
		operator const T& () const
		{
			return *m_obj; // Return the internal object as const reference
		}

		operator T& ()
		{
			return *m_obj; // Return the internal object as non-const reference
		}
		T& operator *() const
		{
			return *m_obj;
		}
		const T& operator()() const 
		{
			return *m_obj; 
		}
		T* GetRealObj() { return m_obj; }
	};
	class XLangException
		: public std::exception
	{
	public:
		int Code()
		{
			return m_code;
		}
	protected:
		int m_code = 0;
	};
	using Str = V<XStr>;
	using Error = V<XError>;
	using Struct = V<XStruct>;
	using Dict = V<XDict>;
	using List = V<XList>;
	//XTensorGraph
	using Tensor = V<XTensor>;
	using TensorGraph = V<XTensorGraph>;
	using Set = V<XSet>;
	using Complex = V<XComplex>;
	using Bin = V<XBin>;
	using Package = V<XPackage>;
	using Event = V<XEvent>;
	using Func = V<XFunc>;
	using XlangClass = V<XLangClass>;
	using PyObject = V<XPyObject>;
	using Runtime = V<XRuntime>;
	using Module = V<XModule>;
}

#endif