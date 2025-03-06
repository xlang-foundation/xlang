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
#include "object.h"
#include "def.h"
#include <vector>
namespace X
{
	class XlangRuntime;
	namespace AST
	{
		class Expression;
	}
	namespace Data
	{
		struct ExpOne
		{
			OP_ID withPrevious =OP_ID::None;//multiple logcial expression with and/or
			OP_ID op;
			long long IndexOfArray=-1;//for${0},0 means current
			X::Value val;
		};
		class ConstExpr :
			virtual public XConstExpr,
			virtual public Object
		{
			OP_ID m_lastCombineOp = OP_ID::None;//for and/or
			std::vector<ExpOne> m_exprs;
			bool m_hasAction = false;
		public:
			ConstExpr():
				Object(),XConstExpr(0)
			{
			}
			bool hasAction() { return m_hasAction; }
			void Set(XlangRuntime* rt,AST::Expression* exp);
			FORCE_INLINE void Run(Object* pObj, long long curPos, bool& retVal)
			{
				retVal = true;
				for (auto& one : m_exprs)
				{
					bool curRetVal = true;
					X::Value itemVal;
					pObj->Get(curPos + one.IndexOfArray, itemVal);
					switch (one.op)
					{
					case OP_ID::Equal:
						curRetVal = (itemVal == one.val);
						break;
					case OP_ID::NotEqual:
						curRetVal = (itemVal != one.val);
						break;
					case OP_ID::Great:
						curRetVal = (itemVal > one.val);
						break;
					case OP_ID::Less:
						curRetVal = (itemVal < one.val);
						break;
					case OP_ID::GreatAndEqual:
						curRetVal = (itemVal >= one.val);
						break;
					case OP_ID::LessAndEqual:
						curRetVal = (itemVal <= one.val);
						break;
					default:
						break;
					}
					if (one.withPrevious == OP_ID::And)
					{
						retVal = retVal && curRetVal;
					}
					else if (one.withPrevious == OP_ID::Or)
					{
						retVal = retVal || curRetVal;
					}
					else
					{
						retVal = curRetVal;
					}
				}
			}
		};
	}
}