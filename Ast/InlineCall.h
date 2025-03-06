/*
Copyright (C) 2024 The XLang Foundation
Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

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