#include "function.h"

namespace X
{
	namespace Data 
	{
		Function::Function(AST::Func* p)
		{
			m_t = Type::Function;
			m_func = p;
		}
		Function::~Function()
		{
		}
		bool Function::Call(Runtime* rt, ARGS& params,
			KWARGS& kwParams,
			AST::Value& retValue)
		{
			return m_func->Call(rt, nullptr, params, kwParams, retValue);
		}
	}
}