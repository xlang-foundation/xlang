#pragma once
#include "exp.h"

namespace X
{
	namespace AST
	{
		extern inline bool ExpExec(Expression* pExp,
			XlangRuntime* rt,
			ExecAction& action,
			XObj* pContext,
			Value& v,
			LValue* lValue = nullptr);
		extern inline bool ExpSet(Expression* pExp,
			XlangRuntime* rt, 
			XObj* pContext, 
			Value& v);
	}
}