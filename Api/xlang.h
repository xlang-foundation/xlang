#ifndef _X_LANG_H_
#define _X_LANG_H_

#include <string>
#include <vector>
#include <unordered_map>
#include "value.h"
#include "xhost.h"

namespace X
{
	enum class ObjType
	{
		Base,
		Str,
		Binary,
		Expr,
		Function,
		MetaFunction,
		XClassObject,
		Prop,
		Event,
		FuncCalls,
		Package,
		ModuleObject,
		Future,
		List,
		Dict,
		TableRow,
		Table,
		RemoteObject,
		PyProxyObject
	};
#define Internal_Reserve(cls_name)  cls_name(int){}

	class XObj
	{
	protected:
		XObj* m_p = nullptr;
	public:
		~XObj()
		{
			if (m_p)
			{
				m_p->DecRef();
			}
		}
		inline operator XObj*() const
		{
			if (m_p)
			{
				m_p->IncRef();
			}
			return m_p;
		}
		virtual int IncRef() { return m_p ? m_p->IncRef() : 0; }
		virtual int DecRef() { return m_p ? m_p->DecRef() : 0; }
		virtual ObjType GetType() { return m_p ? m_p->GetType(): ObjType::Base; }
		virtual std::string GetTypeString() { return m_p ? m_p->GetTypeString() : ""; }
		virtual long long Size() { return m_p ? m_p->Size() : 0; }
		virtual size_t Hash() { return m_p ? m_p->Hash() : 0; }
		virtual std::string ToString(bool WithFormat = false) 
		{
			return m_p ? m_p->ToString() : "";
		}
		virtual bool Call(XRuntime* rt, XObj* pContext,
			ARGS& params,
			KWARGS& kwParams,
			X::Value& retValue)
		{
			return m_p ? m_p->Call(rt, pContext,params, kwParams,retValue) : false;
		}
		virtual bool CallEx(XRuntime* rt, XObj* pContext,
			ARGS& params,
			KWARGS& kwParams,
			X::Value& trailer,
			X::Value& retValue)
		{
			return m_p ? m_p->CallEx(rt, pContext, params, kwParams, trailer,retValue) : false;
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

	};
	class XStr :
		virtual public XObj
	{
	public:
		Internal_Reserve(XStr)
		XStr(const char* data, int size)
		{
			m_p = g_pXHost->CreateStr(data, size);
		}
		virtual char* Buffer() { return m_p ? dynamic_cast<XStr*>(m_p)->Buffer() : nullptr; }
	};
	class XDict :
		virtual public XObj
	{
	public:
		Internal_Reserve(XDict)
		XDict()
		{
			m_p = g_pXHost->CreateDict();
		}
		virtual void Set(X::Value& key, X::Value& val) {
			if (m_p) dynamic_cast<XDict*>(m_p)->Set(key, val);
		}
	};
	class XBin :
		virtual public XObj
	{
	public:
		Internal_Reserve(XBin)
		XBin(char* data, size_t size)
		{
			m_p = g_pXHost->CreateBin(data,size);
		}
		virtual char* Data() { return m_p ? dynamic_cast<XBin*>(m_p)->Data() : nullptr; }
	};
	class XProp :
		virtual public XObj
	{
	
	};
	class XEvent :
		virtual public XObj
	{
	public:
		virtual void DoFire(XRuntime* rt, XObj* pContext, ARGS& params, KWARGS& kwargs) = 0;
	};
	class XFunc:
		virtual public XObj
	{

	};
	class XRemoteObject :
		virtual public XObj
	{
	public:
		virtual void SetObjID(void* id) = 0;
	};
	class XPackage:
		virtual public XObj
	{
	public:
		virtual int AddMethod(const char* name) = 0;
		virtual int QueryMethod(const char* name) = 0;

		virtual void* GetEmbedObj() = 0;
		virtual bool Init(int varNum) = 0;
		virtual bool SetIndexValue(int idx, Value& v) = 0;
		virtual bool GetIndexValue(int idx, Value& v) = 0;
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
	public:
		XPackageValue()
		{
			m_obj = new T();
		}
		XPackageValue(X::Value& v)
		{
			if (v.IsObject())
			{
				X::XPackage* pPack = dynamic_cast<X::XPackage*>(v.GetObj());
				m_obj = (T*)pPack->GetEmbedObj();
			}
		}
		operator Value() const
		{
			if (m_obj)
			{
				X::XPackage* pPackage = m_obj->APISET().GetPack();
				return dynamic_cast<X::XObj*>(pPackage);
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
}

#endif