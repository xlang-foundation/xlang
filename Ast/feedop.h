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
namespace X
{
	namespace AST
	{
		/*
			for other domain-specific language support
			add % at the begin to start a new line also can use line feed \ to crosss multiple
			lines
		*/
		class FeedOp :
			public Operator
		{
		protected:
			char* m_s = nil;
			int m_size = 0;
		public:
			FeedOp(char* s, int size)
			{
				m_type = ObType::FeedOp;
				m_s = s;
				m_size = size;
			}
			FORCE_INLINE virtual bool OpWithOperands(
				std::stack<AST::Expression*>& operands, int LeftTokenIndex)
			{
				operands.push(this);
				return true;
			}
			virtual void ScopeLayout() override
			{

			}
			virtual bool Exec(XlangRuntime* rt,ExecAction& action, XObj* pContext, Value& v, LValue* lValue = nullptr) override;
		};
	}
}