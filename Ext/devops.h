#pragma once
#include "xpackage.h"
#include "function.h"
#include "runtime.h"
#include <vector>
#include <iostream>
namespace X
{
	/*
		About this design pattern
		from assmeblly code,Start function inside mImpl
		will be inlined in released version with optimization 
	*/
	namespace DevOps
	{
		class DebugService
		{
			bool BuildStackInfo(
				Runtime* rt,
				XObj* pContextCurrent,
				AST::CommandInfo* pCommandInfo,
				X::Value& valStackInfo);
			bool BuildLocals(Runtime* rt,
				XObj* pContextCurrent,int frameId,
				X::Value& valLocals);
			bool BuildObjectContent(Runtime* rt,
				XObj* pContextCurrent, int frameId, X::Value& valParam,
				X::Value& valObject);
		public:
			DebugService();
			BEGIN_PACKAGE(DebugService)
				ADD_FUNC("get_startline", GetModuleStartLine)
				ADD_FUNC("set_breakpoints", SetBreakpoints)
				ADD_FUNC("command", Command)
			END_PACKAGE
			bool GetModuleStartLine(void* rt, XObj* pContext,
					ARGS& params,
					KWARGS& kwParams,
					X::Value& retValue);
			bool SetBreakpoints(void* rt, XObj* pContext,
				ARGS& params,
				KWARGS& kwParams,
				X::Value& retValue);
			bool Command(void* rt, XObj* pContext,
				ARGS& params,
				KWARGS& kwParams,
				X::Value& retValue);
		};
	}
}