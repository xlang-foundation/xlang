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