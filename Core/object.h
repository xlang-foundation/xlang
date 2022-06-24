#pragma once
#include <vector>
#include <string>
#include "value.h"
#include "exp.h"
#include "runtime.h"
#include "xclass.h"
#include "module.h"
#include "glob.h"

namespace X {
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
		Dict
	};
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
		Type GetType() { return m_t; }
		virtual bool Call(Runtime* rt, std::vector<AST::Value>& params,
			std::unordered_map<std::string, AST::Value>& kwParams,
			AST::Value& retValue) = 0;
		virtual std::string ToString()
		{
			char v[1000];
			snprintf(v, sizeof(v), "Object:0x%llx",
				(unsigned long long)this);
			return v;
		}
		virtual Object& operator +=(AST::Value& r)
		{
			int x = 1;
			return *this;
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
		virtual bool Call(Runtime* rt, std::vector<AST::Value>& params,
			std::unordered_map<std::string, AST::Value>& kwParams,
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
		virtual bool Call(Runtime* rt, std::vector<AST::Value>& params,
			std::unordered_map<std::string, AST::Value>& kwParams,
			AST::Value& retValue)
		{
			return m_func->Call(rt, nullptr,
				params, kwParams, retValue);
		}
	};

	class Function :
		public Object
	{
	protected:
		AST::Func* m_func = nullptr;
	public:
		Function(AST::Func* p)
		{
			m_t = Type::Function;
			m_func = p;
		}
		AST::Func* GetFunc() { return m_func; }
		virtual bool Call(Runtime* rt, std::vector<AST::Value>& params,
			std::unordered_map<std::string, AST::Value>& kwParams,
			AST::Value& retValue)
		{
			return m_func->Call(rt, nullptr,params, kwParams, retValue);
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
		inline AST::StackFrame* GetStack()
		{
			return m_stackFrame;
		}
		AST::XClass* GetClassObj() { return m_obj; }
		virtual bool Call(Runtime* rt, std::vector<AST::Value>& params,
			std::unordered_map<std::string, AST::Value>& kwParams,
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
		virtual bool Call(Runtime* rt, std::vector<AST::Value>& params,
			std::unordered_map<std::string, AST::Value>& kwParams,
			AST::Value& retValue) override
		{
			return false;
		}
		virtual std::string ToString()
		{
			char v[1000];
			snprintf(v, sizeof(v), "Future:%llu",
				(long long)this);
			return v;
		}
	};

}
}

