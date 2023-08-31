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
		List,
		Dict,
		Tensor,
		TensorExpression,
		TensorOperator,
		TensorGraph,
		Complex,
		Set,
		Struct,
		TableRow,
		Table,
		DeferredObject,
		RemoteObject,
		PyProxyObject
	};

	//For XPackage
	enum class PackageMemberType
	{
		Func,
		FuncEx,
		Prop,
		Const,
		ObjectEvent,
		Class,
		ClassInstance,
	};
#define Internal_Reserve(cls_name)  cls_name(int){}

	class XObj
	{
	protected:
		XRuntime* m_rt = nullptr;
		XObj* m_parent = nullptr;

		inline void SetRT(XRuntime* rt)
		{
			m_rt = rt;
		}
		inline void SetParent(XObj* p)
		{
			m_parent = p;
		}
		
	public:
		XObj()
		{

		}
		~XObj()
		{
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
		virtual XObj* Clone() { return nullptr; }
		virtual bool SupportAssign() { return false; }
		virtual void Assign(const X::Value& val) {}
		virtual int QueryMethod(const char* name, bool* pKeepRawParams = nullptr) { return -1; };
		virtual bool GetIndexValue(int idx, Value& v) { return false; };
		virtual bool Get(XRuntime* rt, XObj* pContext, X::Port::vector<X::Value>& IdxAry, X::Value& val) { return false; }
		virtual int IncRef() { return 0; }
		virtual int DecRef() { return 0; }
		virtual ObjType GetType() { return ObjType::Base; }
		virtual const char* GetTypeString() { return ""; }
		virtual long long Size() { return 0; }
		virtual size_t Hash() { return 0; }
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
		inline virtual Value Member(XRuntime* rt,const char* name)
		{
			X::Value retVal = g_pXHost->QueryMember(rt,this, name);
			if (retVal.IsObject())
			{
				retVal.GetObj()->SetContext(rt, this);
			}
			return retVal;
		}
		inline Value Member(const char* name)
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
		inline X::Value operator()()
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
	};
	class XRuntime :
		virtual public XObj
	{
	public:
		virtual bool CreateEmptyModule() = 0;
		virtual bool AddVar(const char* name, X::Value& val) = 0;
	};
	class XModule :
		virtual public XObj
	{
	public:
		virtual const char* GetFileName() = 0;
		virtual const char* GetPath() = 0;
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
	};
	class XDict :
		virtual public XObj
	{
	public:
		using Dict_Enum = X::Port::Function<void(X::Value& key, X::Value& val)>;
		Internal_Reserve(XDict)
		virtual void Set(X::Value& key, X::Value& val) = 0;
		virtual void Enum(Dict_Enum proc) = 0;
		inline void Set(const char* key, X::Value val)
		{
			X::Value k(key);
			Set(k,val);
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
		virtual char* Data() = 0;
	};

	enum class TensorDataType
	{
		BOOL = 0,
		BYTE, UBYTE,//8 bits
		SHORT, USHORT,//16bits
		HALFFLOAT,//16 bits
		INT, UINT,//32 bits
		LONGLONG, ULONGLONG,//64 bits
		FLOAT, //32 bits
		DOUBLE,//64 bits
		CFLOAT, //32bits+32bits
		CDOUBLE//64 bits+64bits
	};
	class XTensor :
		virtual public XObj
	{
	public:
		Internal_Reserve(XTensor)
		virtual long long GetDataSize() = 0;
		virtual char* GetData() = 0;
		virtual int GetDimCount() = 0;
		virtual long long GetDimSize(int dimIdx) = 0;
		virtual void SetShape(Port::vector<int> shapes) = 0;
		virtual void SetDataType(TensorDataType t) = 0;
		virtual TensorDataType GetDataType() = 0;
		virtual bool Create(X::Value& initData) = 0;
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
	};
	class XProp :
		virtual public XObj
	{
	protected:
		virtual bool SetPropValue(XRuntime* rt0, XObj* pContext, Value& v) = 0;
		virtual bool GetPropValue(XRuntime* rt0, XObj* pContext, Value& v) = 0;
	public:
		Value Get()
		{
			Value v0;
			GetPropValue(m_rt,m_parent, v0);
			return v0;
		}
		bool Set(Value& v)
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

	};
	class XLangClass :
		virtual public XObj
	{
	public:
		virtual int GetBaseClassCount() = 0;
		virtual X::Value GetBaseClass(int idx) = 0;
	};
	class XPyObject :
		virtual public XObj
	{
	public:
		Internal_Reserve(XPyObject);
		virtual bool GetObj(void** ppObjPtr) = 0;
	};
	class XRemoteObject :
		virtual public XObj
	{
	public:
		virtual void SetObjID(unsigned long pid,void* objid) = 0;
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
		virtual int Query(const char* name) = 0;
		virtual bool Get(int idx, X::Value& v) = 0;
		virtual bool Set(int idx, X::Value& v) = 0;
	};
	class XPackage:
		virtual public XObj
	{
	public:
		virtual void SetPackageCleanupFunc(PackageCleanup func) = 0;
		virtual void SetPackageWaitFunc(PackageWaitFunc func) = 0;
		virtual void SetPackageAccessor(PackageAccessor func) = 0;
		virtual int AddMember(PackageMemberType type,const char* name,const char* doc,bool keepRawParams =false) = 0;
		virtual void* GetEmbedObj() = 0;
		virtual bool Init(int varNum) = 0;
		virtual bool SetIndexValue(int idx, Value& v) = 0;
		virtual void RemoveALl() = 0;
		virtual bool RunCodeWithThisScope(const char* code) = 0;
		virtual void SetAPISet(void* pApiSet) = 0;
		virtual bool IsSamePackage(XPackage* pPack) = 0;
	};
	inline long OnEvent(const char* evtName, EventHandler handler)
	{
		return g_pXHost->OnEvent(evtName, handler);
	}
	inline void OffEvent(const char* evtName, long Cookie)
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
		T& operator *() const
		{
			return *m_obj;
		}
		operator T() const
		{
			return *m_obj;
		}
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
	using Struct = V<XStruct>;
	using Dict = V<XDict>;
	using List = V<XList>;
	using Tensor = V<XTensor>;
	using Set = V<XSet>;
	using Complex = V<XComplex>;
	using Bin = V<XBin>;
	using Package = V<XPackage>;
	using Event = V<XEvent>;
	using Func = V<XFunc>;
	using XlangClass = V<XLangClass>;
	using Runtime = V<XRuntime>;
}

#endif