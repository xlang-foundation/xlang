#pragma once
#include "object.h"
#include "PyEngObject.h"
#include "exp.h"
#include "value.h"
#include "runtime.h"
#include "stackframe.h"

namespace X
{
	namespace Data
	{
		//wrap for Python PyObject through PyEng::Object
		class PyProxyObject :
			public Object,
			public AST::Scope
		{
			AST::StackFrame* m_stackFrame = nullptr;
			PyEng::Object m_obj;
		public:
			PyProxyObject()
			{
				m_t = Type::PyProxyObject;
				m_stackFrame = new AST::StackFrame(this);
			}
			PyProxyObject(PyEng::Object& obj):
				PyProxyObject()
			{
				m_obj = obj;
			}
			PyProxyObject(std::string name, std::string path);
			inline virtual AST::Scope* GetScope()
			{
				return dynamic_cast<Scope*>(this);
			}
			// Inherited via Scope
			virtual int AddOrGet(std::string& name, bool bGetOnly) override;
			virtual bool Set(Runtime* rt, void* pContext, 
				int idx, AST::Value& v) override
			{
				m_stackFrame->Set(idx, v);
				return true;
			}
			virtual bool Get(Runtime* rt, void* pContext, 
				int idx, AST::Value& v,
				AST::LValue* lValue = nullptr) override
			{
				m_stackFrame->Get(idx, v, lValue);
				return true;
			}
			virtual Scope* GetParentScope() override
			{
				return nullptr;
			}
			virtual bool Call(Runtime* rt, ARGS& params,
				KWARGS& kwParams, AST::Value& retValue) override;
			virtual std::string ToString(bool WithFormat = false) override
			{
				return (std::string)m_obj;
			}
		};
	}
}