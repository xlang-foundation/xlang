#pragma once
#include "exp.h"
#include "var.h"

//we try to force inline for ExpExec, but because header files including order, 
// can't do it. so leave it here, and use it in exp.cpp
//TODO

#if !defined(FORCE_INLINE_EXP)
#if defined(_MSC_VER)
// Microsoft Visual C++ Compiler
#define FORCE_INLINE_EXP __forceinline
#else
// Fallback for other compilers
#define FORCE_INLINE_EXP 
#endif
#endif

namespace X
{
	namespace AST
	{
		extern FORCE_INLINE_EXP bool ExpExec(Expression* pExp,
			XlangRuntime* rt,
			ExecAction& action,
			XObj* pContext,
			Value& v,
			LValue* lValue = nullptr);
		FORCE_INLINE bool ExpSet(Expression* pExp,
			XlangRuntime* rt, 
			XObj* pContext, 
			Value& v)
		{
			bool bOK = false;
			auto expType = pExp->m_type;
			switch (expType)
			{
			case X::AST::ObType::Var:
				bOK = static_cast<Var*>(pExp)->Set(rt, pContext, v);
				break;
			default:
				break;
			}
			return bOK;
		}
	}
}