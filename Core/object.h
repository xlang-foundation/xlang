#pragma once
#include <vector>
#include <string>
#include "value.h"
#include "exp.h"
#include "runtime.h"
#include "xclass.h"
#include "module.h"
#include "glob.h"
#include "utility.h"
#include "objref.h"
#include "xlang.h"
#include "XLangStream.h"
#include "Locker.h"

namespace X {
	namespace AST { class Scope; }
	typedef X::Value(*EnumProc)(X::Value& elm, unsigned long long idx);
	typedef X::Value(*IterateProc)(
		X::XRuntime* rt, XObj* pContext,
		X::Value& keyOrIdx,X::Value& val,ARGS& params,KWARGS& kwParams);
class Runtime;
namespace Data {
	class List;
	class AttributeBag;
	class Object :
		virtual public XObj,
		virtual public ObjRef
	{
	protected:
		ObjType m_t = ObjType::Base;
		AttributeBag* m_aBag = nullptr;
		Locker m_lock;
	public:
		Object():XObj(), ObjRef()
		{
			G::I().AddObj(this);
		}
		virtual ~Object()
		{
			DeleteAttrBag();
			G::I().RemoveObj(this);
		}
		inline virtual int IncRef()
		{
			AutoLock(m_lock);
			return ObjRef::AddRef();
		}
		virtual void GetBaseScopes(std::vector<AST::Scope*>& bases){}
		AttributeBag* GetAttrBag();
		void DeleteAttrBag();
		inline virtual int DecRef()
		{
			AutoLock(m_lock);
			return ObjRef::Release();
		}
		inline void Lock()
		{
			m_lock.Lock();
		}
		inline void Unlock()
		{
			m_lock.Unlock();
		}
		inline bool IsFunc()
		{
			return (m_t == ObjType::Function);
		}
		inline bool IsStr()
		{
			return (m_t == ObjType::Str);
		}
		virtual List* FlatPack(Runtime* rt,long long startIndex,long long count)
		{ 
			return nullptr; 
		}
		virtual bool Iterate(X::XRuntime* rt, XObj* pContext,
			IterateProc proc,ARGS& params, KWARGS& kwParams)
		{
			return true;
		}
		virtual bool CalcCallables(Runtime* rt, XObj* pContext,
			std::vector<AST::Scope*>& callables)
		{
			return false;
		}
		virtual std::string GetTypeString()
		{
			switch (m_t)
			{
			case X::ObjType::Base:
				return "Base";
			case X::ObjType::Str:
				return "Str";
			case X::ObjType::Binary:
				return "Binary";
			case X::ObjType::Expr:
				return "Expr";
			case X::ObjType::Function:
				return "Function";
			case X::ObjType::MetaFunction:
				return "MetaFunction";
			case X::ObjType::XClassObject:
				return "Class";
			case X::ObjType::FuncCalls:
				return "FuncCalls";
			case X::ObjType::Package:
				return "Package";
			case X::ObjType::Future:
				return "Future";
			case X::ObjType::List:
				return "List";
			case X::ObjType::Dict:
				return "Dict";
			case X::ObjType::TableRow:
				return "TableRow";
			case X::ObjType::Table:
				return "Table";
			case X::ObjType::PyProxyObject:
				return "PyObject";
			default:
				break;
			}
			return "None";
		}
		virtual AST::Scope* GetScope()
		{
			return nullptr;
		}
		virtual ObjType GetType() override
		{
			return m_t;
		}
		virtual long long Size() { return 0; }
		virtual size_t Hash()
		{
			return 0;
		}
		virtual bool ToBytes(Runtime* rt,XObj* pContext,X::XLangStream& stream)
		{ 
			return true; 
		}
		virtual bool FromBytes(X::XLangStream& stream)
		{
			return true;
		}
		virtual std::string ToString(bool WithFormat = false)
		{
			char v[1000];
			snprintf(v, sizeof(v), "Object:0x%llx",
				(unsigned long long)this);
			return v;
		}
		virtual Object& operator +=(X::Value& r)
		{
			return *this;
		}
		virtual Object& operator -=(X::Value& r)
		{
			return *this;
		}
		virtual Object& operator *=(X::Value& r)
		{
			return *this;
		}
		virtual Object& operator /=(X::Value& r)
		{
			return *this;
		}
		virtual int cmp(X::Value* r)
		{
			return 0;
		}
	};
	class Expr
		: public virtual Object
	{//any valid AST tree with one root
	protected:
		AST::Expression* m_expr = nullptr;
	public:
		Expr(AST::Expression* e)
		{
			m_t = ObjType::Expr;
			m_expr = e;
		}
		AST::Expression* Get() { return m_expr; }
		virtual bool Call(XRuntime* rt, XObj* pContext, ARGS& params,
			KWARGS& kwParams,
			X::Value& retValue)
		{
			return true;
		}
	};
	enum class MetaFuncType
	{
		None,
		TaskRun,
	};
	class MetaFunction :
		public virtual Object
	{
	protected:
		AST::Func* m_func = nullptr;
		MetaFuncType m_metaType = MetaFuncType::None;
	public:
		MetaFunction(AST::Func* p, MetaFuncType metaType)
		{
			m_t = ObjType::MetaFunction;
			m_metaType = metaType;
			m_func = p;
		}
		AST::Func* GetFunc() { return m_func; }
		virtual bool Call(XRuntime* rt, XObj* pContext, ARGS& params,
			KWARGS& kwParams,
			X::Value& retValue)
		{
			return m_func->Call(rt, nullptr,
				params, kwParams, retValue);
		}
	};

	class Future:
		public virtual Object
	{
		void* m_pTask = nullptr;
	public:
		Future()
		{
			m_t = ObjType::Future;
		}
		Future(void* task)
			:Future()
		{
			m_pTask = task;
		}
		virtual bool Call(XRuntime* rt, XObj* pContext, ARGS& params,
			KWARGS& kwParams,
			X::Value& retValue) override
		{
			return false;
		}
		virtual std::string ToString(bool WithFormat = false)
		{
			char v[1000];
			snprintf(v, sizeof(v), "Future:%llu",
				(long long)this);
			return v;
		}
	};

}
}

