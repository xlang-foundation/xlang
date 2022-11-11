#include "feedop.h"
#include "runtime.h"

namespace X
{
	namespace AST
	{
		bool FeedOp::Run(XlangRuntime* rt, XObj* pContext, Value& v, LValue* lValue)
		{
			std::string info;
			RunStringExpWithFormat(rt, pContext, m_s + 1, m_size - 1, info);
			//std::cout << info << std::endl;
			X::Value valIndex;
			X::Value valInfo(info);
			rt->CallWritePads(valInfo, valIndex);
			return true;
		}
	}
}