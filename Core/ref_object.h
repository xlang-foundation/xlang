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
#include "xlang.h"
#include "object.h"
#include <string>

// Forward declaration of AST::Expression
namespace X {
    namespace AST {
        class Expression;
    }
}

namespace X
{
    namespace Data
    {
        class RefObject :
            virtual public XRef,
            virtual public Object
        {
        public:
            RefObject() :XRef(0), Object()
            {
				m_t = ObjType::Ref;
            }
            // Constructor accepting an AST::Expression pointer.
            RefObject(AST::Expression* expr)
                : XRef(0), Object(),m_expression(expr)
            {
                m_t = ObjType::Ref;
            }

            // Virtual destructor.
            virtual ~RefObject() = default;

            // Accessor for the stored expression.
            AST::Expression* GetExpression() const
            {
                return m_expression;
            }

            // Optionally override ToString for debugging or representation.
            virtual const char* ToString(bool WithFormat = false) override
            {
                return "RefObject";
            }

            // If needed, provide a Clone implementation.
            virtual XObj* Clone() override
            {
                // Shallow copy; m_expression is not deep-copied.
                return new RefObject(m_expression);
            }
            virtual X::Value Apply() override
            {
                return X::Value();
            }
            virtual bool Call(XRuntime* rt, XObj* pContext,
                ARGS& params,
                KWARGS& kwParams,
                X::Value& retValue)
            {
				if (m_expression)
				{
					XlangRuntime* xrt = dynamic_cast<XlangRuntime*>(rt);
					AST::ExecAction action;
					X::Value v;
					bool bOK = ExpExec(m_expression, xrt, action, pContext, v);
					if (bOK)
					{
						retValue = v;
					}
					return bOK;
				}
                return true;
            }
        private:
            AST::Expression* m_expression = nullptr;
        };
    }
}