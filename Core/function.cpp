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
		bool Function::Call(XRuntime* rt, XObj* pContext,
			ARGS& params,
			KWARGS& kwParams,
			X::Value& retValue)
		{
			return m_func->Call(rt, pContext, params, kwParams, retValue);
		}
		bool Function::CallEx(XRuntime* rt, XObj* pContext, 
			ARGS& params, KWARGS& kwParams, X::Value& trailer, X::Value& retValue)
		{
			return m_func->CallEx(rt, pContext, params, kwParams, trailer,retValue);
		}
	}
}