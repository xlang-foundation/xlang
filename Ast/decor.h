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
#include "op.h"
#include "var.h"

namespace X
{
	class XlangRuntime;
	namespace AST
	{
		class Expression;
		class Decorator :
			public UnaryOp
		{
			Expression* m_client = nullptr;
			bool GetParamList(XlangRuntime* rt, Expression* e, ARGS& params, KWARGS& kwParams);
			bool RunExp(XlangRuntime* rt,Value& v, LValue* lValue);
		public:
			Decorator() :
				UnaryOp()
			{
				m_type = ObType::Decor;
			}
			Decorator(short op) :
				UnaryOp(op)
			{
				m_type = ObType::Decor;
			}
			~Decorator()
			{

			}
			//for syntax: @jit(...)
			FORCE_INLINE bool IsJitDecorator()
			{
				if (R)
				{
					auto ty = R->m_type;
					switch (ty)
					{
					case ObType::Pair:
					{
						auto* pBinOp = (BinaryOp*)R;
						if (pBinOp->GetL()->m_type == ObType::Var)
						{
							auto* pVar = (Var*)pBinOp->GetL();
							if (pVar->GetNameString() == "jit")
							{
								return true;
							}
						}
					}
					break;
					case ObType::Var:
					{
						auto* pVar = (Var*)R;
						if (pVar->GetNameString() == "jit")
						{
							return true;

						}
					}
					break;
					default:
						break;
					}
				}
				return false;
			}
			FORCE_INLINE Expression* Client() { return m_client; }
			FORCE_INLINE void SetClient(Expression* e) { m_client = e; }
			virtual bool Exec(XlangRuntime* rt,ExecAction& action, XObj* pContext, Value& v, LValue* lValue = nullptr) override;
		};
	}
}