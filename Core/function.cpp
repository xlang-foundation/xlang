#include "function.h"

namespace X
{
	namespace Data 
	{
		Function::Function(AST::Func* p, bool bOwnIt)
		{
			m_ownFunc = bOwnIt;
			m_t = ObjType::Function;
			m_func = p;
		}
		Function::~Function()
		{
			if (m_ownFunc && m_func)
			{
				delete m_func;
			}
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