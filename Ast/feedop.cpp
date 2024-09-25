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

#include "feedop.h"
#include "runtime.h"

namespace X
{
	namespace AST
	{
		bool FeedOp::Exec(XlangRuntime* rt,ExecAction& action, XObj* pContext, Value& v, LValue* lValue)
		{
			int padCount = 0;
			int padUsingDataBindingCount = 0;
			rt->GetWritePadNum(padCount, padUsingDataBindingCount);
			std::string fmtString;
			std::string fmtBindingString;
			std::vector<Value> Value_Bind_list;
			if (padCount > padUsingDataBindingCount)
			{
				RunStringExpWithFormat(rt, pContext, m_s + 1, m_size - 1, fmtString,
					false, Value_Bind_list);
			}
			if (padUsingDataBindingCount)
			{
				RunStringExpWithFormat(rt, pContext, m_s + 1, m_size - 1, fmtBindingString,
					true, Value_Bind_list);
			}
			X::Value valIndex;
			X::Value valFmtInfo = fmtString;
			X::Value valFmtBindInfo = fmtBindingString;
			rt->CallWritePads(valFmtInfo, valFmtBindInfo,valIndex, Value_Bind_list);
			return true;
		}
	}
}