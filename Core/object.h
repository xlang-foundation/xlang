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
class XlangRuntime;
namespace Data {
	class List;
	class AttributeBag;
	typedef void* Iterator_Pos;
	class Object :
		virtual public XObj,
		virtual public ObjRef
	{
	protected:
		ObjType m_t = ObjType::Base;
		AttributeBag* m_aBag = nullptr;
		AST::Scope* m_extraScope = nullptr;//ref to extra scope, don't delete it
		Locker m_lock;
		Locker m_external_lock;
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
			AutoLock autoLock(m_lock);
			return ObjRef::AddRef();
		}
		inline void SetExtraScope(AST::Scope* pScope)
		{
			m_extraScope = pScope;
		}
		virtual XObj* Clone() override
		{
			IncRef();
			return this;
		}
		inline virtual bool GetAndUpdatePos(Iterator_Pos& pos, std::vector<Value>& vals)
		{
			return true;
		}
		inline virtual void CloseIterator(Iterator_Pos pos) {}
		inline virtual void GetBaseScopes(std::vector<AST::Scope*>& bases)
		{
			if (m_extraScope)
			{
				bases.push_back(m_extraScope);
			}
		}
		AttributeBag* GetAttrBag();
		void DeleteAttrBag();
		inline virtual int DecRef()
		{
			m_lock.Lock();
			int ref = ObjRef::Release();
			if (ref == 0)
			{
				m_lock.Unlock();
				delete this;
			}
			else
			{
				m_lock.Unlock();
			}
			return ref;
		}
		inline void ExternLock()
		{
			m_external_lock.Lock();
		}
		inline void ExternUnlock()
		{
			m_external_lock.Unlock();
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
		inline std::string CombinObjectIds(std::vector<std::string>& IdList,
			std::string newId)
		{
			return concat(IdList, ".") + "." + newId;
		}
		inline std::string CombinObjectIds(std::vector<std::string>& IdList,
			unsigned long long id)
		{
			return concat(IdList, ".") + "." + ::tostring(id);
		}
		inline bool IsStr()
		{
			return (m_t == ObjType::Str);
		}
		virtual List* FlatPack(XlangRuntime* rt, XObj* pContext,
			std::vector<std::string>& IdList, int id_offset,
			long long startIndex,long long count)
		{ 
			return nullptr; 
		}
		virtual X::Value UpdateItemValue(XlangRuntime* rt, XObj* pContext,
			std::vector<std::string>& IdList, int id_offset,
			std::string itemName,X::Value& val)
		{
			return val;
		}
		virtual bool Iterate(X::XRuntime* rt, XObj* pContext,
			IterateProc proc,ARGS& params, KWARGS& kwParams, 
			X::Value& retValue)
		{
			return true;
		}
		virtual bool CalcCallables(XlangRuntime* rt, XObj* pContext,
			std::vector<AST::Scope*>& callables)
		{
			return false;
		}
		//GetTypeString will cross dll, so,return a pointer not std::string
		//and this point will released by call g_pHost func 
		virtual const char* GetTypeString() override
		{
			std::string typeName = GetObjectTypeString();
			return GetABIString(typeName);
		}
		//todo: should impl. into individual class  with virtual
		inline std::string GetObjectTypeString()
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
			case X::ObjType::Prop:
				return "Prop";
			case X::ObjType::Future:
				return "Future";
			case X::ObjType::List:
				return "List";
			case X::ObjType::Dict:
				return "Dict";
			case X::ObjType::Set:
				return "Set";
			case X::ObjType::Tensor:
			case X::ObjType::TensorExpression:
				return "Tensor";
			case X::ObjType::TableRow:
				return "TableRow";
			case X::ObjType::Table:
				return "Table";
			case X::ObjType::RemoteObject:
				return "RemoteObject";
			case X::ObjType::PyProxyObject:
				return "PyObject";
			default:
				break;
			}
			return "None";
		}
		virtual bool wait(int timeout)
		{
			return true;
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
		virtual bool ToBytes(XlangRuntime* rt,XObj* pContext,X::XLangStream& stream)
		{ 
			return true; 
		}
		virtual bool FromBytes(X::XLangStream& stream)
		{
			return true;
		}
		virtual const char* ToString(bool WithFormat = false) override
		{
			char v[1000];
			snprintf(v, sizeof(v), "Object:0x%llx",
				(unsigned long long)this);
			std::string retStr(v);
			return GetABIString(retStr);
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
		//if it is equal, return 0
		virtual int cmp(X::Value* r)
		{
			if (r->GetType() == ValueType::None ||
				r->GetType() == ValueType::Invalid)
			{
				return 1;//object is nor None,
			}
			return 0;
		}
		virtual bool Get(long long idx, X::Value& val) { return false; }
		virtual bool Set(long long idx, X::Value& val) { return false; }
		virtual bool Set(Value valIdx, X::Value& val) { return false; }
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
			if (m_expr)
			{
				AST::ExecAction action;
				return m_expr->Exec((XlangRuntime*)rt, action, pContext, retValue);
			}
			else
			{
				return true;
			}
		}
		virtual bool ToBytes(XlangRuntime* rt, XObj* pContext, X::XLangStream& stream)
		{
			AST::Expression exp;
			exp.SaveToStream(rt, pContext, m_expr, stream);
			return true;
		}
		virtual bool FromBytes(X::XLangStream& stream)
		{
			AST::Expression exp;
			m_expr = exp.BuildFromStream<AST::Expression>(stream);
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

}
}

