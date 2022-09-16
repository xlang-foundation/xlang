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
			XPackageAPISet<DebugService> m_Apis;
		public:
			XPackageAPISet<DebugService>& APISET() { return m_Apis; }
			DebugService();
			int GetModuleStartLine(unsigned long long moduleKey);
			X::Value SetBreakpoints(X::XRuntime* rt,X::XObj* pContext,
				unsigned long long moduleKey, Value& varLines);
			bool Command(X::XRuntime* rt, X::XObj* pContext,
				ARGS& params,
				KWARGS& kwParams,
				X::Value& retValue);
		};
	}
}