#pragma once

#include "xpackage.h"
#include "xlang.h"

namespace X 
{
	class JsonWrapper
	{
	public:
		BEGIN_PACKAGE(JsonWrapper)
			ADD_FUNC("loads", LoadFromString)
			ADD_FUNC("load", LoadFromFile)
		END_PACKAGE
		JsonWrapper()
		{

		}
		bool LoadFromString(void* rt, XObj* pContext,
			ARGS& params,
			KWARGS& kwParams,
			X::Value& retValue);
		bool LoadFromFile(void* rt, XObj* pContext,
			ARGS& params,
			KWARGS& kwParams,
			X::Value& retValue);
	};
}