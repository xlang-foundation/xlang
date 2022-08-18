#include "function.h"

namespace X
{
	namespace Data 
	{
		Function::Function(AST::Func* p)
		{
			m_t = ObjType::Function;
			m_func = p;
		}
		Function::~Function()
		{
		}
		bool Function::Call(XRuntime* rt, ARGS& params,
			KWARGS& kwParams,
			X::Value& retValue)
		{
			return m_func->Call(rt, nullptr, params, kwParams, retValue);
		}
	}
}