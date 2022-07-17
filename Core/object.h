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

namespace X {
	namespace AST { class Scope; }
class Runtime;
namespace Data {
	enum class Type
	{
		Base,
		Str,
		Binary,
		Expr,
		Function,
		MetaFunction,
		XClassObject,
		FuncCalls,
		Package,
		Future,
		List,
		Dict,
		TableRow,
		Table,
		PyProxyObject
	};
	class List;
	class Object
	{
	protected:
		int m_ref = 0;
		Type m_t = Type::Base;
	public:
		Object()
		{
			G::I().AddObj(this);
		}
		virtual ~Object()
		{
			G::I().RemoveObj(this);
		}
		inline bool IsFunc()
		{
			return (m_t == Type::Function);
		}
		inline bool IsStr()
		{
			return (m_t == Type::Str);
		}
		virtual List* FlatPack(Runtime* rt,long long startIndex,long long count)
		{ 
			return nullptr; 
		}
		virtual bool CalcCallables(Runtime* rt, void* pContext,
			std::vector<AST::Scope*>& callables) 
		{
			return false;
		}
		std::string GetTypeString()
		{
			switch (m_t)
			{
			case X::Data::Type::Base:
				return "Base";
			case X::Data::Type::Str:
				return "Str";
			case X::Data::Type::Binary:
				return "Binary";
			case X::Data::Type::Expr:
				return "Expr";
			case X::Data::Type::Function:
				return "Function";
			case X::Data::Type::MetaFunction:
				return "MetaFunction";
			case X::Data::Type::XClassObject:
				return "Class";
			case X::Data::Type::FuncCalls:
				return "FuncCalls";
			case X::Data::Type::Package:
				return "Package";
			case X::Data::Type::Future:
				return "Future";
			case X::Data::Type::List:
				return "List";
			case X::Data::Type::Dict:
				return "Dict";
			case X::Data::Type::TableRow:
				return "TableRow";
			case X::Data::Type::Table:
				return "Table";
			default:
				break;
			}
			return "None";
		}
		inline int AddRef() 
		{ 
			return ++m_ref; 
		}
		inline int Release() 
		{ 
			int ref = --m_ref;
			if (ref == 0)
			{
				delete this;
			}
			return ref;
		}
		virtual AST::Scope* GetScope()
		{
			return nullptr;
		}
		Type GetType() { return m_t; }
		virtual bool Call(Runtime* rt, ARGS& params,
			KWARGS& kwParams,
			AST::Value& retValue) = 0;
		virtual long long Size() { return 0; }
		virtual size_t Hash()
		{
			return 0;
		}
		virtual std::string ToString(bool WithFormat = false)
		{
			char v[1000];
			snprintf(v, sizeof(v), "Object:0x%llx",
				(unsigned long long)this);
			return v;
		}
		virtual Object& operator +=(AST::Value& r)
		{
			return *this;
		}
		virtual Object& operator -=(AST::Value& r)
		{
			return *this;
		}
		virtual Object& operator *=(AST::Value& r)
		{
			return *this;
		}
		virtual Object& operator /=(AST::Value& r)
		{
			return *this;
		}
		virtual int cmp(AST::Value* r)
		{
			return 0;
		}
	};
	class Expr
		:public Object
	{//any valid AST tree with one root
	protected:
		AST::Expression* m_expr = nullptr;
	public:
		Expr(AST::Expression* e)
		{
			m_t = Type::Expr;
			m_expr = e;
		}
		AST::Expression* Get() { return m_expr; }
		virtual bool Call(Runtime* rt, ARGS& params,
			KWARGS& kwParams,
			AST::Value& retValue)
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
		public Object
	{
	protected:
		AST::Func* m_func = nullptr;
		MetaFuncType m_metaType = MetaFuncType::None;
	public:
		MetaFunction(AST::Func* p, MetaFuncType metaType)
		{
			m_t = Type::MetaFunction;
			m_metaType = metaType;
			m_func = p;
		}
		AST::Func* GetFunc() { return m_func; }
		virtual bool Call(Runtime* rt, ARGS& params,
			KWARGS& kwParams,
			AST::Value& retValue)
		{
			return m_func->Call(rt, nullptr,
				params, kwParams, retValue);
		}
	};

	class XClassObject :
		public Object
	{
	protected:
		AST::XClass* m_obj = nullptr;
		AST::StackFrame* m_stackFrame = nullptr;
	public:
		XClassObject(AST::XClass* p)
		{
			m_t = Type::XClassObject;
			m_obj = p;
			m_stackFrame = new AST::StackFrame((AST::Scope*)this);
			m_stackFrame->SetVarCount(p->GetVarNum());
			auto* pClassStack = p->GetClassStack();
			if (pClassStack)
			{
				m_stackFrame->Copy(pClassStack);
			}
		}
		~XClassObject()
		{
			if (m_stackFrame)
			{
				delete m_stackFrame;
			}
		}
		virtual long long Size() override
		{ 
			return m_obj?m_obj->GetVarNum():0;
		}
		virtual List* FlatPack(Runtime* rt,
			long long startIndex, long long count) override;
		virtual bool CalcCallables(Runtime* rt, void* pContext,
			std::vector<AST::Scope*>& callables) override
		{
			return m_obj ? m_obj->CalcCallables(rt, pContext, callables) : false;
		}
		virtual std::string ToString(bool WithFormat = false)
		{
			char v[1000];
			snprintf(v, sizeof(v), "Class:%s@0x%llx",
				m_obj->GetNameString().c_str(), (unsigned long long)this);
			return v;
		}
		inline AST::StackFrame* GetStack()
		{
			return m_stackFrame;
		}
		AST::XClass* GetClassObj() { return m_obj; }
		virtual bool Call(Runtime* rt, ARGS& params,
			KWARGS& kwParams,
			AST::Value& retValue)
		{
			return m_obj->Call(rt,this,
				params, kwParams, retValue);
		}
	};
	class Future:
		public Object
	{
		void* m_pTask = nullptr;
	public:
		Future()
		{
			m_t = Type::Future;
		}
		Future(void* task)
			:Future()
		{
			m_pTask = task;
		}
		virtual bool Call(Runtime* rt, ARGS& params,
			KWARGS& kwParams,
			AST::Value& retValue) override
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

