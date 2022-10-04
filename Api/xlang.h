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

	struct XObj_Context
	{
		XRuntime* rt = nullptr;
		XObj* m_parent = nullptr;
		XObj* m_this = nullptr;
	};
	class XObj
	{
	protected:
		XObj_Context* m_cxt = nullptr;
		inline XObj& SetRT(XRuntime* rt)
		{
			m_cxt->rt = rt;
			return *this;
		}
		inline XObj& SetParent(XObj* p)
		{
			m_cxt->m_parent = p;
			if (m_cxt->m_parent)
			{
				m_cxt->m_parent->IncRef();
			}
			return *this;
		}
		
	public:
		XObj()
		{

		}
		XObj(X::Value& v0)
		{
			m_cxt = new XObj_Context;
			XObj* pObj = (XObj*)v0;
			m_cxt->m_this = pObj;
			if (pObj->m_cxt)
			{
				m_cxt->rt = pObj->m_cxt->rt;
				m_cxt->m_parent = pObj->m_cxt->m_parent;
				if (m_cxt->m_parent)
				{
					m_cxt->m_parent->IncRef();
				}
			}
		}
		XObj(XObj* p)
		{//callee need to addref for p
			m_cxt = new XObj_Context;
			m_cxt->m_this = p;
			if (p->m_cxt)
			{
				m_cxt->rt = p->m_cxt->rt;
				m_cxt->m_parent = p->m_cxt->m_parent;
				if (m_cxt->m_parent)
				{
					m_cxt->m_parent->IncRef();
				}
			}
		}
		~XObj()
		{
			if (m_cxt)
			{
				if (m_cxt->m_this)
				{
					m_cxt->m_this->DecRef();
				}
				if (m_cxt->m_parent)
				{
					m_cxt->m_parent->DecRef();
				}
				delete m_cxt;
			}
		}
		void SetContext(XRuntime* rt, XObj* pa)
		{
			if (m_cxt)
			{
				if (m_cxt->m_this)
				{
					m_cxt->m_this->DecRef();
				}
				if (m_cxt->m_parent)
				{
					m_cxt->m_parent->DecRef();
				}
				delete m_cxt;
			}
			m_cxt = new XObj_Context();
			m_cxt->rt = rt;
			m_cxt->m_parent = pa;
			if (pa)
			{
				pa->IncRef();
			}
		}
		XObj(const XObj& self)
		{
			if (self.m_cxt)
			{
				m_cxt = new XObj_Context;
				m_cxt->rt = self.m_cxt->rt;
				m_cxt->m_this = self.m_cxt->m_this;
				if (m_cxt->m_this)
				{
					m_cxt->m_this->IncRef();
				}
				m_cxt->m_parent = self.m_cxt->m_parent;
				if (m_cxt->m_parent)
				{
					m_cxt->m_parent->IncRef();
				}
			}
		}
		XObj_Context* GetContext() { return m_cxt; }
		XObj& operator=(const XObj& o)
		{
			if (m_cxt)
			{
				if (m_cxt->m_this)
				{
					m_cxt->m_this->DecRef();
				}
				if (m_cxt->m_parent)
				{
					m_cxt->m_parent->DecRef();
				}
				delete m_cxt;
			}
			if (o.m_cxt)
			{
				m_cxt = new XObj_Context;
				m_cxt->rt = o.m_cxt->rt;
				m_cxt->m_this = o.m_cxt->m_this;
				if (m_cxt->m_this)
				{
					m_cxt->m_this->IncRef();
				}
				m_cxt->m_parent = o.m_cxt->m_parent;
				if (m_cxt->m_parent)
				{
					m_cxt->m_parent->IncRef();
				}
			}
			return *this;
		}
		inline bool WithContext() { return m_cxt != nullptr; }
		inline XObj* This() { return m_cxt->m_this; }
		inline operator XObj*() const
		{
			XObj* p = nullptr;
			if (m_cxt)
			{
				if (m_cxt->m_this)
				{
					m_cxt->m_this->IncRef();
					p = m_cxt->m_this;
				}
			}
			return p;
		}
		virtual int IncRef() { return m_cxt ? m_cxt->m_this->IncRef() : 0; }
		virtual int DecRef() { return m_cxt ? m_cxt->m_this->DecRef() : 0; }
		virtual ObjType GetType() { return m_cxt ? m_cxt->m_this->GetType(): ObjType::Base; }
		virtual std::string GetTypeString() { return m_cxt ? m_cxt->m_this->GetTypeString() : ""; }
		virtual long long Size() { return m_cxt ? m_cxt->m_this->Size() : 0; }
		virtual size_t Hash() { return m_cxt ? m_cxt->m_this->Hash() : 0; }
		virtual std::string ToString(bool WithFormat = false) 
		{
			return m_cxt ? m_cxt->m_this->ToString() : "";
		}
		virtual bool Call(XRuntime* rt, XObj* pContext,
			ARGS& params,
			KWARGS& kwParams,
			X::Value& retValue)
		{
			return WithContext() ? This()->Call(rt, pContext, params, kwParams, retValue) : false;
		}
		virtual bool CallEx(XRuntime* rt, XObj* pContext,
			ARGS& params,
			KWARGS& kwParams,
			X::Value& trailer,
			X::Value& retValue)
		{
			return WithContext() ? This()->CallEx(rt, pContext, params, kwParams, trailer,retValue) : false;
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
		inline XObj Member(XRuntime* rt,const char* name)
		{
			return XObj(g_pXHost->QueryMember(rt, m_cxt ? m_cxt->m_this : this, name))
				.SetRT(rt).SetParent(m_cxt->m_this);
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
			Call(m_cxt->rt,m_cxt->m_parent,params, kwargs,v0);
			if (v0.IsObject())
			{
				v0.GetObj()->SetContext(m_cxt->rt, m_cxt->m_parent);
			}
			return v0;
		}
		inline X::Value operator()()
		{
			X::ARGS params;
			X::KWARGS kwargs;
			X::Value v0;
			Call(m_cxt->rt, m_cxt->m_parent, params, kwargs, v0);
			if (v0.IsObject())
			{
				v0.GetObj()->SetContext(m_cxt->rt, m_cxt->m_parent);
			}
			return v0;
		}
		inline XObj operator[](const char* key)
		{
			return Member(m_cxt->rt,key);
		}
	};
	class XStr :
		virtual public XObj
	{
	public:
		Internal_Reserve(XStr)
		XStr(const char* data, int size):
			XObj(g_pXHost->CreateStr(data, size))
		{
		}
		virtual char* Buffer() { return WithContext() ? dynamic_cast<XStr*>(This())->Buffer() : nullptr; }
	};
	class XDict :
		virtual public XObj
	{
	public:
		Internal_Reserve(XDict)
		XDict():
			XObj(g_pXHost->CreateDict())
		{
		}
		virtual void Set(X::Value& key, X::Value& val) {
			if (WithContext()) dynamic_cast<XDict*>(This())->Set(key, val);
		}
	};
	class XBin :
		virtual public XObj
	{
	public:
		Internal_Reserve(XBin)
		XBin(char* data, size_t size):
			XObj(g_pXHost->CreateBin(data, size))
		{
		}
		virtual char* Data() { return WithContext() ? dynamic_cast<XBin*>(This())->Data() : nullptr; }
	};
	class XProp :
		virtual public XObj
	{
	protected:
		virtual bool SetProp(XRuntime* rt0, XObj* pContext, Value& v) = 0;
		virtual bool GetProp(XRuntime* rt0, XObj* pContext, Value& v) = 0;
	public:
		Value Get()
		{
			Value v0;
			GetProp(m_cxt->rt, m_cxt->m_parent, v0);
			return v0;
		}
		bool Set(Value& v)
		{
			SetProp(m_cxt->rt, m_cxt->m_parent, v);
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
		virtual int AddMethod(const char* name,bool keepRawParams =false) = 0;
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