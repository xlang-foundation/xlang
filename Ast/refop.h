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
#include "op.h"
#include "exp_exec.h"
#include "ref_object.h"

/*
	Test code
	test/type/refop.x
*/

namespace X
{
	namespace AST
	{
		class RefOp :
			public UnaryOp
		{
		public:
			RefOp() :
				UnaryOp()
			{
				m_type = ObType::RefOp;
			}
			RefOp(short op) :
				UnaryOp(op)
			{
				m_type = ObType::RefOp;
			}
			// Finished Exec implementation
			FORCE_INLINE virtual bool Exec(XlangRuntime* rt, ExecAction& action,
				XObj* pContext, Value& v, LValue* lValue = nullptr) override
			{
				// Get the operand expression from UnaryOp
				Expression* operand = GetR();
				if (!operand)
					return false;
				// Create a new RefObject using the operand's value.
				// Assuming RefObject has a constructor that takes a Value.
				Data::RefObject* refObj = new Data::RefObject(operand);
				
				// Assign the new RefObject to the output value.
				v = X::Value(refObj);
				return true;
			}
		};
	}
}